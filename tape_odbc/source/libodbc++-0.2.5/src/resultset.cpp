/*
   This file is part of libodbc++.

   Copyright (C) 1999-2000 Manush Dodunekov <manush@stendahls.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <odbc++/resultset.h>
#include <odbc++/statement.h>
#include <odbc++/resultsetmetadata.h>

#include "datahandler.h"
#include "datastream.h"
#include "driverinfo.h"
#include "dtconv.h"

#if defined(ODBCXX_QT)
# include <qiodevice.h>
#endif

using namespace odbc;
using namespace std;

// values for location_
#define BEFORE_FIRST      (-3)
#define AFTER_LAST        (-2)
#define INSERT_ROW        (-1)
#define UNKNOWN           (0)

#define LOCATION_IS_VALID (location_>=0)

// ---------------------------------------------------
/* 
 * Hack to fix problems with updates on Rowset's when there are Steramed columns that have been touched.
 * If not, they will be truncated or throw parameter error during updateRow();
 * ##&&## ToDo:
 * better stream handling
 */
#define HACK_BYPASS_UPDATE_BUG
// ---------------------------------------------------
/*
  Our current location in a result set is stored in location_. If
  location_>0, we know our position. If it's UNKNOWN we don't
  but it's still somewhere in the resultset. If location_<0
  we're outside or on the insert row.

  We decide on which of the SQL*Fetch* functions to use in the
  following way:

  if (DM_IS_ODBC3) {
    if(forwardOnly) {
      SQLFetch();
    } else {
      SQLFetchScroll();
    }
  } else {
    if(forwardOnly && rowsetSize==1) {
      SQLFetch();
    } else {
      SQLExtendedFetch();
    }
  }


  For scrollable ResultSets, we add an extra row to rowset_. A call to
  moveToInsertRow() would put the rowset position on the last row.

  When using an ODBC2 driver, a call to insertRow simply does
  _applyPosition(SQL_ADD) (while on the insert row).

  With an ODBC3 driver, we have to re-bind all columns to point at the
  last row with rowset size set to 1 (stupid ODBC bind offsets only work with
  row-wise binding, and we use column-wise), and then call SQLBulkOperations.
  That binding is kept in effect until moveToCurrentRow() is called, for
  the case of the application wanting to insert more rows.
  When calling moveToCurrentRow() in this case, we have two possibilities:

  1. bindPos_ = 0 -> app didn't call insertRow(), we can just set location_
  to locBeforeInsert_ and the rowset pos to rowBeforeInsert_.

  2. bindPos_ >0 -> app inserted one or more rows using SQLBulkOperations().
  Per ODBC3 specs, the actual rowset position is undefined, and we do the
  following:
     1. Restore the rowset size (SQL_ROW_ARRAY_SIZE)
     2. If locBeforeInsert_ was valid, use an absolute fetch to get there and
     restore the rowset size. Otherwise, try to restore it by going after-last
     or before-first. With a driver that can't report the current row position
     (eg returns 0 for SQLGetStmtAttr(SQL_ATTR_ROW_NUMBER)), this breaks and
     leaves a ResultSet before it's first row after an insert. However, I haven't
     seen an ODBC3 driver that does this yet.
*/




namespace odbc {
  // a simple utility to save a value and restore it
  // at the end of the scope
  template <class T> class ValueSaver {
  private:
    T& orig_;
    T val_;
  public:
    ValueSaver(T& v)
      :orig_(v), val_(v) {}
    ~ValueSaver() {
      orig_=val_;
    }
  };
};



#define CHECK_INSERT_ROW                                        \
do {                                                                \
  if(location_==INSERT_ROW) {                                        \
    throw SQLException                                                \
      (ODBCXX_STRING_CONST("[libodbc++]: Illegal operation while on insert row"), ODBCXX_STRING_CONST("HY010")); \
  }                                                                \
} while(false)


#define CHECK_SCROLLABLE_CURSOR                                                \
do {                                                                        \
  if(this->getType()==TYPE_FORWARD_ONLY) {                                \
    throw SQLException                                                        \
      (ODBCXX_STRING_CONST("[libodbc++]: Operation not possible on a forward-only cursor"), ODBCXX_STRING_CONST("HY010"));        \
  }                                                                         \
} while(false)


ResultSet::ResultSet(Statement* stmt, SQLHSTMT hstmt, bool ownStmt)
  :statement_(stmt),
   hstmt_(hstmt),
   ownStatement_(ownStmt),
   currentFetchSize_(stmt->getFetchSize()),
   newFetchSize_(currentFetchSize_),
   rowset_(NULL),
   rowStatus_(NULL),
   rowsInRowset_(0),
   colsBound_(false),
   streamedColsBound_(false),
   bindPos_(0),
   location_(BEFORE_FIRST),
   supportsGetDataAnyOrder_(false)
{
  metaData_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ResultSetMetaData(this);

  //ODBC says GetData cannot be called on a forward-only cursor
  //with rowset size > 1
  //so, if ResultSetMetaData saw a streamed column,
  //we 'adjust' the rowset size to 1
  if(metaData_->needsGetData_==true &&
     this->getType()==TYPE_FORWARD_ONLY) {
    currentFetchSize_=1;
  }

  newFetchSize_=currentFetchSize_;
  
  supportsGetDataAnyOrder_ = this->_getDriverInfo()->supportsGetDataAnyOrder();
/*
#ifdef WIN32
if(!supportsGetDataAnyOrder_)
{	
	int cnt = metaData_->getColumnCount();
	for(int i = 1; i < cnt;i++)
	{	if(metaData_->getColumnType(i) == Types::LONGVARCHAR)
		{	throw SQLException (ODBCXX_STRING_CONST("[libodbc++]: Driver does not support \'GetData() Any_Order\', Text/LongVarChar is not the last"), SQLException::scDEFSQLSTATE);	}
	} // for
}
#endif  
*/
#if ODBCVER >= 0x0300

  // with ODBC3, we call SQLFetchScroll,
  // and need to set the following
  statement_->_setPointerOption
    (SQL_ATTR_ROWS_FETCHED_PTR,(SQLPOINTER)&rowsInRowset_);

#endif

  //this will do all the work - set the rowset size, bind and so on
  this->_applyFetchSize();
}

ResultSet::~ResultSet()
{
  if(colsBound_) {
    this->_unbindCols();
  }

  // in case some exception caused us
  // to destruct, these can be bound
  if(streamedColsBound_) {
    this->_unbindStreamedCols();
  }

#if ODBCVER >= 0x0300

  // we don't want anything to point to us
  statement_->_setPointerOption
    (SQL_ATTR_ROWS_FETCHED_PTR,(SQLPOINTER)NULL);

  statement_->_setPointerOption
    (SQL_ATTR_ROW_STATUS_PTR,(SQLPOINTER)NULL);

#endif

  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) rowset_;
  delete[] rowStatus_;
  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) metaData_;
  statement_->_unregisterResultSet(this);

  //if we own the statement, nuke it
  if(ownStatement_) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) statement_;
  }
}

//private
void ResultSet::_applyFetchSize()
{
  statement_->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_ROW_ARRAY_SIZE,SQL_ROWSET_SIZE),
     currentFetchSize_);

  // it is possible that the driver changes the
  // rowset size, so we do an extra check
  int driverFetchSize=statement_->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_ROW_ARRAY_SIZE,SQL_ROWSET_SIZE));

  if(driverFetchSize!=currentFetchSize_) {
    // save these values
    currentFetchSize_=driverFetchSize;
    newFetchSize_=driverFetchSize;
  }


  if(colsBound_) {
    this->_unbindCols();
  }

  this->_resetRowset();


  if(!colsBound_) {
    this->_bindCols();
  }
}

//private
void ResultSet::_resetRowset()
{
  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) rowset_;
  delete[] rowStatus_;

  //Forward-only resultsets can't have insert rows
  int extraRows=(this->getType()==TYPE_FORWARD_ONLY?0:1);
  rowset_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) Rowset(currentFetchSize_+extraRows,//1 for the insert row
                     ODBC3_DC(true,false)); // possibly use ODBC3 c-types

  rowStatus_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) SQLUSMALLINT[currentFetchSize_+extraRows]; //same here

#if ODBCVER >= 0x0300
  // since SQLFetchScroll doesn't take this argument, we
  // need to set it here.
  statement_->_setPointerOption
    (SQL_ATTR_ROW_STATUS_PTR,(SQLPOINTER)rowStatus_);

#endif

  //add the datahandlers
  int nc=metaData_->getColumnCount();
  for(int i=1; i<=nc; i++) {
    int realprec;
#if ODBCVER < 0x0300

    realprec=metaData_->getPrecision(i);

#else

    if(this->_getDriverInfo()->getMajorVersion()>=3) {

      switch(metaData_->getColumnType(i)) {
      case Types::CHAR:
      case Types::VARCHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H)
          case Types::WCHAR:
          case Types::WVARCHAR:
#endif
      case Types::BINARY:
      case Types::VARBINARY:
        realprec=metaData_->colLengths_[i-1];
        break;
      default:
        realprec=metaData_->getPrecision(i);
        break;
      }

    } else {
      realprec=metaData_->getPrecision(i);
    }

#endif

    rowset_->addColumn(metaData_->getColumnType(i),
                       realprec,
                       metaData_->getScale(i));
  }
}

//private
void ResultSet::_prepareForFetch()
{
  if(newFetchSize_!=currentFetchSize_) {
    currentFetchSize_=newFetchSize_;
    this->_applyFetchSize();
  }
}

SQLRETURN ResultSet::_applyPosition(int mode)
{
  if(this->getType() != TYPE_FORWARD_ONLY) {
    SQLRETURN r=SQLSetPos(hstmt_,
                          (SQLUSMALLINT)rowset_->getCurrentRow()+1,
                          (SQLUSMALLINT)mode,
                          SQL_LOCK_NO_CHANGE);

    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("SQLSetPos failed"), ODBCXX_STRING_CONST("HY109"));
    return r;
  }
  return SQL_SUCCESS;
}


void ResultSet::_bindCols()
{
  int nc=metaData_->getColumnCount();

  SQLRETURN r;

  bindPos_=rowset_->getCurrentRow();
  colsBound_=true;

  for(int i=1; i<=nc; i++) {
    DataHandler* dh=rowset_->getColumn(i);
    if(!dh->isStreamed_)
    {
      r=SQLBindCol(hstmt_,
                   (SQLUSMALLINT)i,
                   dh->cType_,
                   (SQLPOINTER)dh->data(),
                   dh->bufferSize_,
                   &dh->dataStatus_[dh->currentRow_]);

      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error binding column"));
    }
/*
// Trying to allocate and bind Streamed column for drivers that doesn't support out-of-order 
// Data fetch, not working correctly, Anyone ??
    else if(!supportsGetDataAnyOrder_)
    {
		if(dh->stream_ == NULL)
		{	  
			ODBCXX_STREAM* s=dh->getStream();
#if defined(ODBCXX_HAVE_SQLUCODE_H)
		    s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataStream(this,hstmt_,i,SQL_C_TCHAR,
#else
		    s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataStream(this,hstmt_,i,SQL_C_CHAR,
#endif
						 dh->dataStatus_[dh->currentRow_]);
			dh->setStream(s);
		}
		streamedColsBound_=true;
		r=SQLBindCol(hstmt_,
                   (SQLUSMALLINT)i,
                   dh->cType_,
                   (SQLPOINTER)dh->data(),
                   dh->bufferSize_,
                   &dh->dataStatus_[dh->currentRow_]);

      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error Streamed binding column"));
    }
*/    
  }
}
// --------------------------------------------------------
inline bool isEmpty(ODBCXX_STREAM* stream)
{
	bool retVal = (stream == NULL);
	if(!retVal)
	{
#if defined(ODBCXX_QT)
        retVal = stream->atEnd();
#else
		retVal = stream->eof();
#endif
	}

	return retVal;
}
// --------------------------------------------------------
//this we do before an update or insert
void ResultSet::_bindStreamedCols()
{
  int nc=metaData_->getColumnCount();
  unsigned int cr=rowset_->getCurrentRow();

  for(int i=1; i<=nc; i++) {
    DataHandler* dh=rowset_->getColumn(i);
// JR - I had a case where the Stream was NULL...
    if(dh->isStreamed_ )
	{
		if(!isEmpty(dh->stream_)) 
		{ // && dh-> != NULL
		  streamedColsBound_=true;
		  SQLRETURN r=SQLBindCol(hstmt_,
								 (SQLUSMALLINT)i,
								 dh->cType_,
								 (SQLPOINTER)i, //our column number (for SQLParamData)
								 0,
								 &dh->dataStatus_[dh->currentRow_]);
		  //dh->dataStatus_ should be SQL_LEN_DATA_AT_EXEC(len) by this point
		  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error binding column"));
		}
	}
  }
}

void ResultSet::_unbindStreamedCols()
{
  int nc=metaData_->getColumnCount();
  streamedColsBound_=false;

  for(int i=1; i<=nc; i++) {
    DataHandler* dh=rowset_->getColumn(i);
    if(dh->isStreamed_) {
      SQLRETURN r=SQLBindCol(hstmt_,
                           (SQLUSMALLINT)i,
                           dh->cType_,
                           0, //setting this to NULL means unbind this column
                           0,
                           dh->dataStatus_);
      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error unbinding column"));
    }
  }
}



//private
void ResultSet::_unbindCols()
{
  //we don't perform an error check here, since
  //this is called from the destructor;
  //egcs dislikes exceptions thrown from destructors.
  SQLFreeStmt(hstmt_,SQL_UNBIND);
  colsBound_=false;
}


//private
//all actual fetching is done here

void ResultSet::_doFetch(int fetchType, int rowNum)
{
  SQLRETURN r;

  bool isfo=this->getType()==TYPE_FORWARD_ONLY;

  // if isfo is true fetchType can only be SQL_FETCH_NEXT
  if(isfo
#if ODBCVER < 0x0300
     && currentFetchSize_==1
#endif
     ) {
    // the only way to get here
    assert(fetchType==SQL_FETCH_NEXT);

    // ODBC3 says SQLFetch can do rowset sizes > 1
    r=SQLFetch(hstmt_);
#if ODBCVER < 0x0300
    // this won't get updated otherwise
    rowsInRowset_=1;
#endif

  } else {

    // we have either a scrolling cursor or a
    // rowset size > 1 in ODBC2
#if ODBCVER < 0x0300
    r=SQLExtendedFetch(hstmt_,
                       (SQLUSMALLINT)fetchType,
                       (SQLINTEGER)rowNum,
                       &rowsInRowset_,
                       rowStatus_);

#else

    r=SQLFetchScroll(hstmt_,
                     (SQLUSMALLINT)fetchType,
                     (SQLINTEGER)rowNum);

#endif

  }


  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching data from datasource"));

  rowset_->setCurrentRow(0);

  // fixup for weird behaviour experienced with myodbc 2.50.28:
  // while on the first row of the result set, a call to SQLExtendedFetch(SQL_FETCH_PRIOR)
  // would set rowsInRowset_ to 0 but return SQL_SUCCESS_WITH_INFO.
  if(rowsInRowset_==0 &&
     r!=ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    r=ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND);
  }

  if(r==ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    rowsInRowset_=0;
    switch(fetchType) {
    case SQL_FETCH_RELATIVE:
      if(rowNum<0) {
        location_=BEFORE_FIRST;
      } else if(rowNum>0) {
        location_=AFTER_LAST;
      }
      break;

    case SQL_FETCH_ABSOLUTE:
      if(rowNum==0) {
        location_=BEFORE_FIRST;
      } else {
        location_=AFTER_LAST;
      }
      break;

    case SQL_FETCH_NEXT:
    case SQL_FETCH_LAST:
      location_=AFTER_LAST;
      break;

    case SQL_FETCH_FIRST:
    case SQL_FETCH_PRIOR:
      location_=BEFORE_FIRST;
      break;

    default:
      //notreached
      assert(false);
    }
  } else {

    // for forward only cursors, we don't bother fetching the location
    // we just increment it for each fetch

    if(isfo) {
      if(location_<=0) {
        location_=1;
      } else {
        location_+=currentFetchSize_;
      }
    } else {
      SQLUINTEGER l=statement_->_getUIntegerOption
        (ODBC3_C(SQL_ATTR_ROW_NUMBER,SQL_ROW_NUMBER));

      if(l==0) {
        location_=UNKNOWN;
      } else {
        location_=(int)l;
      }
    }
  }
}
// --------------------------------------------------------

const char* ODBC_HY090 = "HY090";
const char* ODBC_22026 = "22026";

#define CHECK_ERR(ret, resultset, hstmt, fromwhere)	if(ret != SQL_SUCCESS) {resultset->_checkStmtError(hstmt,ret,ODBCXX_STRING_CONST(fromwhere));}
#define CHECK_ERR_SQLSTATE(ret, resultset, hstmt, fromwhere, ODBCSTATE)	if(ret != SQL_SUCCESS) {resultset->_checkStmtError(hstmt,ret,ODBCXX_STRING_CONST(fromwhere), ODBCXX_STRING_CONST(ODBCSTATE));}
// --------------------------------------------------------
void ResultSet::_handleStreams(SQLRETURN r)
{
  if(r==SQL_NEED_DATA) {
    ODBCXX_CHAR_TYPE buf[PUTDATA_CHUNK_SIZE];

    while(r==SQL_NEED_DATA) {
      SQLPOINTER currentCol;
      r=SQLParamData(hstmt_,&currentCol);
//      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("SQLParamData failure"));
	  CHECK_ERR_SQLSTATE(r, this, hstmt_, "SQLParamData failure", SQLException::scDEFSQLSTATE)
      if(r==SQL_NEED_DATA) {
        DataHandler* dh=rowset_->getColumn(*static_cast<SQLUINTEGER *>(currentCol));

        assert(dh->isStreamed_);

        int charsPut=0;

        ODBCXX_STREAM* s=dh->getStream();
//		if(s != NULL)
//		{	s->seekg(0, ios_base::beg);}
        int b;
        while((b=readStream(s,buf,PUTDATA_CHUNK_SIZE))>0) {
          charsPut+=b;
          SQLRETURN rPD=SQLPutData(hstmt_,(SQLPOINTER)buf,b*sizeof(ODBCXX_CHAR_TYPE));
//          this->_checkStmtError(hstmt_,rPD,ODBCXX_STRING_CONST("SQLPutData failure"));
		  CHECK_ERR_SQLSTATE(r, this, hstmt_, "SQLPutData failure", ODBC_HY090)
        }

        if(charsPut==0) {
          // SQLPutData has not been called
          SQLRETURN rPD=SQLPutData(hstmt_,(SQLPOINTER)buf,0);
//			SQLRETURN rPD=SQLPutData(hstmt_,(SQLPOINTER)buf,SQL_NULL_DATA);
//          this->_checkStmtError(hstmt_,rPD,ODBCXX_STRING_CONST("SQLPutData(0) failure"));
		  CHECK_ERR_SQLSTATE(r, this, hstmt_, "SQLPutData(0) failure", ODBC_HY090)
        }
      }
    }
  }
}




int ResultSet::getType()
{
  return statement_->getResultSetType();
}

int ResultSet::getConcurrency()
{
  return statement_->getResultSetConcurrency();
}

void ResultSet::setFetchSize(int fs)
{
  CHECK_INSERT_ROW;

  if(fs==0) {
    fs=SQL_ROWSET_SIZE_DEFAULT; //1
  }

  if(fs>0) {
    newFetchSize_=fs;
    if(fs!=currentFetchSize_) {
      //ok, we'll need to change it
      //if we are before/after the resultset, it's ok to proceed now
      if(!LOCATION_IS_VALID) {
        currentFetchSize_=fs;
        this->_applyFetchSize();
      }
    }
  } else {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid fetch size ")+intToString(fs), ODBCXX_STRING_CONST("HY024"));
  }
}


bool ResultSet::next()
{
  CHECK_INSERT_ROW;

  if(LOCATION_IS_VALID &&
     rowset_->getCurrentRow()+1 < rowsInRowset_) {
    //we just move inside the rowset
    rowset_->setCurrentRow(rowset_->getCurrentRow()+1);
    this->_applyPosition();
    return true;
  } else if(location_==AFTER_LAST) {
    //no more to come
    return false;
  }

  //we now know for sure we have to do a fetch
  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_NEXT,0);

  if(LOCATION_IS_VALID) {
    this->_applyPosition();
    return true;
  }

  return false;
}

bool ResultSet::previous()
{
  CHECK_INSERT_ROW;

  CHECK_SCROLLABLE_CURSOR;


  if(LOCATION_IS_VALID &&
     rowset_->getCurrentRow()>0) {

    //alright, just move inside the rowset
    rowset_->setCurrentRow(rowset_->getCurrentRow()-1);
    this->_applyPosition();
    return true;

  } else if(location_==BEFORE_FIRST) {

    //no rows there..
    return false;

  } else {

    //if, we have, say a rowset that covers rows 10-30, and
    //we request the previous, we get rows 1-20. Then
    //we'll have to adjust to position to the
    //right row

    int oldloc=location_;

    this->_prepareForFetch();
    this->_doFetch(SQL_FETCH_PRIOR,0);

    if(LOCATION_IS_VALID) {
      if(oldloc>0 &&
         oldloc-location_ < currentFetchSize_) {
        rowset_->setCurrentRow(oldloc-2); //1 for 1-based location, 1 for previous
      } else {
        //just the last row in the rowset
        rowset_->setCurrentRow(rowsInRowset_-1);
      }
      this->_applyPosition();
      return true;
    } else {
      return false;
    }
  }
}


bool ResultSet::first()
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  // don't trust any positions, just fetch the first rowset
  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_FIRST,0);

  if(LOCATION_IS_VALID) {
    this->_applyPosition();
    return true;
  }

  return false;
}


bool ResultSet::last()
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_LAST,0);

  if(LOCATION_IS_VALID) {
    rowset_->setCurrentRow(rowsInRowset_-1);
    this->_applyPosition();
    return true;
  } else {
    return false;
  }
}

bool ResultSet::relative(int rows)
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  if(!LOCATION_IS_VALID) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: ResultSet::relative(): no current row"), ODBCXX_STRING_CONST("HY107"));
  }

  if(rows==0) {
    return true;
  }

  if(rows>0 && rowset_->getCurrentRow() < rowsInRowset_-rows) {
    rowset_->setCurrentRow(rowset_->getCurrentRow()+rows);
  } else if(rows<0 && rowset_->getCurrentRow() >= -rows) {
    rowset_->setCurrentRow(rowset_->getCurrentRow()+rows);
  } else {

    //since SQL_FETCH_RELATIVE is relative to the start of the rowset
    //we'll need to adjust
    rows-=rowset_->getCurrentRow();

    this->_prepareForFetch();
    this->_doFetch(SQL_FETCH_RELATIVE,rows);
  }

  if(LOCATION_IS_VALID) {
    this->_applyPosition();
    return true;
  } else {
    return false;
  }
}


bool ResultSet::absolute(int row)
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  this->_prepareForFetch();

  this->_doFetch(SQL_FETCH_ABSOLUTE,row);

  if(LOCATION_IS_VALID) {
    this->_applyPosition();
    return true;
  }

  return false;
}

void ResultSet::beforeFirst()
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  if(location_!=BEFORE_FIRST) {
    this->absolute(0);
  }
}

void ResultSet::afterLast()
{
  CHECK_INSERT_ROW;
  CHECK_SCROLLABLE_CURSOR;

  if(location_!=AFTER_LAST) {
    this->_prepareForFetch();

    this->_doFetch(SQL_FETCH_LAST,0); //now on last rowset
    this->_doFetch(SQL_FETCH_NEXT,0); //now after last rowset
  }
}



void ResultSet::moveToInsertRow()
{
  CHECK_SCROLLABLE_CURSOR;

  if(location_!=INSERT_ROW) {
    rowBeforeInsert_=rowset_->getCurrentRow();
    locBeforeInsert_=location_;

    rowset_->setCurrentRow(currentFetchSize_);


    location_=INSERT_ROW;
  }
}

void ResultSet::moveToCurrentRow()
{
  CHECK_SCROLLABLE_CURSOR;

  if(location_==INSERT_ROW) {

    if(bindPos_>0) {
      assert(this->_getDriverInfo()->getMajorVersion()>2);
      // this occurs after we have inserted rows
      // with an ODBC3 driver
      // restore our fetch size and
      // rebind with the real positions
      statement_->_setUIntegerOption
        (ODBC3_C(SQL_ATTR_ROW_ARRAY_SIZE,SQL_ROWSET_SIZE),
         currentFetchSize_);

      rowset_->setCurrentRow(0);
      this->_bindCols();

      // this mirrors rowset_->getCurrentRow()
      assert(bindPos_==0);

      if(locBeforeInsert_>0) {
        // we know our remembered position
        this->_doFetch(SQL_FETCH_ABSOLUTE, locBeforeInsert_);
      } else {

        switch(locBeforeInsert_) {
        case AFTER_LAST:
          this->_doFetch(SQL_FETCH_ABSOLUTE,-1);
          this->_doFetch(SQL_FETCH_NEXT,0);
          break;

        case BEFORE_FIRST:
        case UNKNOWN: // inconsistent
        default:
          this->_doFetch(SQL_FETCH_ABSOLUTE,0);
        }
      }

    } else {

      // we are in the right rowset already
      location_=locBeforeInsert_;

    }

    if(LOCATION_IS_VALID) {
      rowset_->setCurrentRow(rowBeforeInsert_);
      this->_applyPosition();
    } else {
      rowset_->setCurrentRow(0);
    }
  }
}

void ResultSet::insertRow()
{
  CHECK_SCROLLABLE_CURSOR;

  if(location_==INSERT_ROW) {

    // first, decide whether we use SQLBulkOperations
    // or SQLSetPos
#if ODBCVER >= 0x0300
    if(this->_getDriverInfo()->getMajorVersion()==3) {

      if(bindPos_==0) {
        // this is the first time we call insertRow()
        // while on the insert row
        statement_->_setUIntegerOption
          (ODBC3_C(SQL_ATTR_ROW_ARRAY_SIZE,SQL_ROWSET_SIZE),1);
        // this would bind only the last row of our rowset
        this->_bindCols();

        assert(bindPos_>0);
      }

      this->_bindStreamedCols();

      SQLRETURN r=SQLBulkOperations(hstmt_,SQL_ADD);

      try {
        this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("SQLBulkOperations failed"));
      } catch(...) {
        this->_unbindStreamedCols();
        throw;
      }

      this->_handleStreams(r);

      this->_unbindStreamedCols();

      rowset_->afterUpdate();
      return;
    }
#endif

    // we are using SQLSetPos(SQL_ADD) to insert a row
    SQLRETURN r;
    this->_bindStreamedCols();

    try {
      r=this->_applyPosition(SQL_ADD);
    } catch(...) {
      this->_unbindStreamedCols();
      throw;
    }

    this->_handleStreams(r);
    this->_unbindStreamedCols();
    rowset_->afterUpdate();

  } else {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Not on insert row"), ODBCXX_STRING_CONST("HY109"));
  }
}

void ResultSet::updateRow()
{
  CHECK_SCROLLABLE_CURSOR;

  CHECK_INSERT_ROW;

  if(!LOCATION_IS_VALID) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: No current row"), ODBCXX_STRING_CONST("HY109"));
  }

  this->_bindStreamedCols();
  // With ODBC3, the operation below will change our
  // rowsInRowset_, so we save it
  SQLRETURN r;
  {
#if ODBCVER >= 0x0300
    ValueSaver<SQLUINTEGER> _rows(rowsInRowset_);
#endif

    r=this->_applyPosition(SQL_UPDATE);
  }

  this->_handleStreams(r);


  this->_unbindStreamedCols();

  rowset_->afterUpdate();
}

void ResultSet::refreshRow()
{
  CHECK_SCROLLABLE_CURSOR;

  CHECK_INSERT_ROW;

  if(!LOCATION_IS_VALID) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: No current row"), ODBCXX_STRING_CONST("HY109"));
  }
  this->_applyPosition(SQL_REFRESH);
}


void ResultSet::deleteRow()
{
  CHECK_SCROLLABLE_CURSOR;

  CHECK_INSERT_ROW;
  if(!LOCATION_IS_VALID) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: No current row"), ODBCXX_STRING_CONST("HY109"));
  }

  {
#if ODBCVER >= 0x0300
    ValueSaver<SQLUINTEGER> _rows(rowsInRowset_);
#endif

    this->_applyPosition(SQL_DELETE);
  }
}


void ResultSet::cancelRowUpdates()
{
  CHECK_SCROLLABLE_CURSOR;

  if(LOCATION_IS_VALID) {
    // do a refresh
    this->refreshRow();
  } else if(location_==INSERT_ROW) {
    // this will set all values to NULL
    rowset_->afterUpdate();
  } else {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: No current row"), ODBCXX_STRING_CONST("HY109"));
  }
}


int ResultSet::findColumn(const ODBCXX_STRING& colName)
{
	int retVal = -1;
#ifdef ODBCXX_USE_INDEXED_METADATA_COLNAMES
	if((retVal = metaData_->findColumn(colName)) > -1)
	{	return retVal;}
#else

  for(int i=1; i<=metaData_->getColumnCount(); i++) {
    if(
#if !defined(WIN32)
# if defined(ODBCXX_UNICODE)
       wcscasecmp
# else
       strcasecmp
# endif
#elif defined(ODBCXX_HAVE__STRICMP)
# if defined(ODBCXX_UNICODE)
       _wcsicmp
# else
       _stricmp
# endif
#elif defined(ODBCXX_HAVE_STRICMP)
# if defined(ODBCXX_UNICODE)
       wcsicmp
# else
       stricmp
# endif
#else
# error Cannot determine case-insensitive string compare function
#endif
       (ODBCXX_STRING_CSTR(colName),
        ODBCXX_STRING_CSTR(metaData_->getColumnName(i)))==0) {
      retVal = i;
    }
  }
#endif // ODBCXX_USE_INDEXED_METADATA_COLNAMES

  throw SQLException
    (ODBCXX_STRING_CONST("[libodbc++]: Column ")+colName+ODBCXX_STRING_CONST(" not found in result set"), ODBCXX_STRING_CONST("42S22"));

  //shut up MSVC
  return retVal;
}

int ResultSet::getRow()
{
  if(location_>0 && rowsInRowset_>0) {
    return location_+rowset_->getCurrentRow();
  } else if(location_==INSERT_ROW && locBeforeInsert_>0) {
    return locBeforeInsert_+rowBeforeInsert_;
  }

  return 0;
}


ODBCXX_STRING ResultSet::getCursorName()
{
  ODBCXX_CHAR_TYPE buf[256];
  SQLSMALLINT t;
  SQLRETURN r=SQLGetCursorName(hstmt_,
                               (ODBCXX_SQLCHAR*)buf,
                               255,
                               &t);

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching cursor name"));

  buf[255]=0;
  return ODBCXX_STRING_C(buf);
}


bool ResultSet::rowDeleted()
{
  return (rowStatus_[rowset_->getCurrentRow()-bindPos_]==SQL_ROW_DELETED);
}

bool ResultSet::rowInserted()
{
  return (rowStatus_[rowset_->getCurrentRow()-bindPos_]==SQL_ROW_ADDED);
}

bool ResultSet::rowUpdated()
{
  return (rowStatus_[rowset_->getCurrentRow()-bindPos_]==SQL_ROW_UPDATED);
}


bool ResultSet::isAfterLast()
{
  return location_==AFTER_LAST;
}

bool ResultSet::isBeforeFirst()
{
  return location_==BEFORE_FIRST;
}

bool ResultSet::isFirst()
{
  CHECK_INSERT_ROW;

  if(!LOCATION_IS_VALID) {
    return false;
  }

  if(location_!=UNKNOWN) {
    return (location_==1 && rowset_->getCurrentRow()==0);
  }

  // to do more, we need scrolling
  CHECK_SCROLLABLE_CURSOR;

  // we have to do a fetch
  // as with isLast(), fall down to rowset size=1
  int oldfs=currentFetchSize_;
  newFetchSize_=1;
  int oldpos=rowset_->getCurrentRow();

  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_PREV,0);

  bool havePrev=LOCATION_IS_VALID;

  newFetchSize_=oldfs;
  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_NEXT,0);
  rowset_->setCurrentRow(oldpos);
  this->_applyPosition();

  return !havePrev;
}


bool ResultSet::isLast()
{
  CHECK_INSERT_ROW;

  if(!LOCATION_IS_VALID ||
     rowset_->getCurrentRow() < rowsInRowset_-1) {
    //we know for sure
    return false;
  }

  //nothing we can do if we aren't scrolling
  CHECK_SCROLLABLE_CURSOR;

  //here, we fall down to a rowset size of 1 in order to
  //be able to come back to exactly the same position

  int oldfs=currentFetchSize_;
  int oldpos=rowset_->getCurrentRow();
  newFetchSize_=1;

  this->_prepareForFetch();
  this->_doFetch(SQL_FETCH_NEXT,0);

  bool haveMore=LOCATION_IS_VALID;

  newFetchSize_=oldfs;

  this->_prepareForFetch();

  // return to our position
  this->_doFetch(SQL_FETCH_PREV,0);
  rowset_->setCurrentRow(oldpos);
  this->_applyPosition();

  return !haveMore;
}



#define CHECK_COL(x)                                        \
do {                                                        \
  if(x<1 || x>metaData_->getColumnCount()) {                \
    throw SQLException                                        \
      (ODBCXX_STRING_CONST("Column index out of range"), ODBCXX_STRING_CONST("42S12"));\
  }                                                        \
} while(false)


// this allows the position to be on the insert row
#define CHECK_LOCATION                                                \
do {                                                                \
  if(!LOCATION_IS_VALID && location_!=INSERT_ROW) {                \
    throw SQLException                                                \
      (ODBCXX_STRING_CONST("[libodbc++]: No current row"), ODBCXX_STRING_CONST("HY109"));        \
  }                                                                \
} while(false)


#define IMPLEMENT_GET(RETTYPE,FUNCSUFFIX)                        \
RETTYPE ResultSet::get##FUNCSUFFIX(int idx)                        \
{                                                                \
  CHECK_COL(idx);                                                \
  CHECK_LOCATION;                                               \
  DataHandler* dh=rowset_->getColumn(idx);                        \
  lastWasNull_=dh->isNull();                                        \
  return dh->get##FUNCSUFFIX();                                        \
}                                                                \
                                                                \
RETTYPE ResultSet::get##FUNCSUFFIX(const ODBCXX_STRING& colName)\
{                                                                \
  return this->get##FUNCSUFFIX(this->findColumn(colName));        \
}


#define IMPLEMENT_UPDATE(TYPE,FUNCSUFFIX)                                \
void ResultSet::update##FUNCSUFFIX(int idx, TYPE val)                        \
{                                                                        \
  CHECK_COL(idx);                                                        \
  CHECK_LOCATION;                                                       \
  DataHandler* dh=rowset_->getColumn(idx);                                \
  dh->set##FUNCSUFFIX(val);                                                \
}                                                                        \
                                                                        \
void ResultSet::update##FUNCSUFFIX(const ODBCXX_STRING& colName, TYPE val)\
{                                                                        \
  this->update##FUNCSUFFIX(this->findColumn(colName),val);                \
}

#define IMPLEMENT_BOTH(TYPE,FUNCSUFFIX)                \
IMPLEMENT_GET(TYPE,FUNCSUFFIX);                        \
IMPLEMENT_UPDATE(TYPE,FUNCSUFFIX)


IMPLEMENT_BOTH(double,Double);
IMPLEMENT_BOTH(bool,Boolean);
IMPLEMENT_BOTH(signed char,Byte);
IMPLEMENT_BOTH(float,Float);
IMPLEMENT_BOTH(int,Int);
IMPLEMENT_BOTH(Long,Long);
IMPLEMENT_BOTH(short,Short);

//darn, this didn't fit in
//IMPLEMENT_GET(ODBCXX_BYTES,Bytes);
//IMPLEMENT_UPDATE(const ODBCXX_BYTES&, Bytes);
//IMPLEMENT_GET(ODBCXX_STRING,String);
//IMPLEMENT_UPDATE(const ODBCXX_STRING&, String);
IMPLEMENT_GET(Date,Date);
IMPLEMENT_UPDATE(const Date&, Date);
IMPLEMENT_GET(Time,Time);
IMPLEMENT_UPDATE(const Time&, Time);
IMPLEMENT_GET(Timestamp,Timestamp);
IMPLEMENT_UPDATE(const Timestamp&, Timestamp);

void ResultSet::updateString(int idx, const ODBCXX_STRING& str)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
  DataHandler* dh=rowset_->getColumn(idx);
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  if((dh->getSQLType()!=Types::LONGVARCHAR)&&(dh->getSQLType()!=Types::WLONGVARCHAR)) {
#else
  if((dh->getSQLType()!=Types::LONGVARCHAR)) {
#endif
    dh->setString(str);
  } else {
    dh->setStream(stringToStream(str), str.length(), true);
  }
}

void ResultSet::updateString(const ODBCXX_STRING& colName,
                             const ODBCXX_STRING& str)
{
  this->updateString(this->findColumn(colName),str);
}

ODBCXX_STRING ResultSet::getString(int idx)
{
  CHECK_COL(idx);
  CHECK_LOCATION;

  DataHandler* dh=rowset_->getColumn(idx);
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  if((dh->getSQLType()!=Types::LONGVARCHAR)&&(dh->getSQLType()!=Types::WLONGVARCHAR)) {
#else
  if((dh->getSQLType()!=Types::LONGVARCHAR)) {
#endif
    lastWasNull_=dh->isNull();
    return dh->getString();
  } else {	
    // lazily fetch the stream as a string
	  // return streamToString(this->getAsciiStream(idx));
          ODBCXX_STRING retVal( streamToString(this->getAsciiStream(idx)) );
#ifdef HACK_BYPASS_UPDATE_BUG
	  dh->resetStream();
#endif // HACK_BYPASS_UPDATE_BUG
#ifdef CACHE_STREAMED_DATA
//	  size_t txtlen = retVal.size();
//	  dh->setupBuffer(txtlen+3);
//	  memcpy(dh->data(), retVal.c_str(), txtlen);
#endif // CACHE_STREAMED_DATA
	return retVal;
  }
}
/*
fudgeBuffer(const char* str, )
{
	getRows()
}
*/
ODBCXX_STRING ResultSet::getString(const ODBCXX_STRING& colName)
{
  return this->getString(this->findColumn(colName));
}


void ResultSet::updateBytes(int idx, const ODBCXX_BYTES& b)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
  DataHandler* dh=rowset_->getColumn(idx);
  if(dh->getSQLType()!=Types::LONGVARBINARY) {
    dh->setBytes(b);
  } else {
    dh->setStream(bytesToStream(b));
  }
}

void ResultSet::updateBytes(const ODBCXX_STRING& colName,
                            const ODBCXX_BYTES& b)
{
  this->updateBytes(this->findColumn(colName),b);
}

ODBCXX_BYTES ResultSet::getBytes(int idx)
{
  CHECK_COL(idx);
  CHECK_LOCATION;

  DataHandler* dh=rowset_->getColumn(idx);
  if(dh->getSQLType()!=Types::LONGVARBINARY) {
    lastWasNull_=dh->isNull();
    return dh->getBytes();
  } else {
//    return streamToBytes(this->getBinaryStream(idx));
	  ODBCXX_BYTES retVal = streamToBytes(this->getBinaryStream(idx));
#ifdef HACK_BYPASS_UPDATE_BUG
	  dh->resetStream();
#endif // HACK_BYPASS_UPDATE_BUG
	return retVal;

  }
}

ODBCXX_BYTES ResultSet::getBytes(const ODBCXX_STRING& colName)
{
  return this->getBytes(this->findColumn(colName));
}

#ifdef ODBCXX_HAVE_STRUCT_GUID
odbc::Guid ResultSet::getGuid(int idx) 
{
	CHECK_COL(idx);
	CHECK_LOCATION;

	DataHandler* dh=rowset_->getColumn(idx);
	if (dh->getSQLType() == Types::GUID ) {
		lastWasNull_=dh->isNull();
		return dh->getGuid();
	} else {
		throw SQLException("ResultSet::getGuid from non GUID not implemented");
	}
}
#endif

//nor did this
void ResultSet::updateNull(int idx)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
  rowset_->getColumn(idx)->setNull();
}

void ResultSet::updateNull(const ODBCXX_STRING& colName)
{
  this->updateNull(this->findColumn(colName));
}


//or this
ODBCXX_STREAM* ResultSet::getAsciiStream(int idx)
{
  CHECK_COL(idx);
  CHECK_LOCATION;

  // we can't get the stream of the insert row
//  CHECK_INSERT_ROW;

  //if the stream is not created yet, we create it
  DataHandler* dh=rowset_->getColumn(idx);
  ODBCXX_STREAM* s=dh->getStream();
  if(s==NULL && location_!=INSERT_ROW) {
#if defined(ODBCXX_HAVE_SQLUCODE_H)
    s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataStream(this,hstmt_,idx,SQL_C_TCHAR,
#else
    s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataStream(this,hstmt_,idx,SQL_C_CHAR,
#endif
                     dh->dataStatus_[dh->currentRow_]);
    dh->setStream(s);
  }
//  else 
//  {	dh->setNull();}
  lastWasNull_=dh->isNull();
  return s;
}

ODBCXX_STREAM* ResultSet::getAsciiStream(const ODBCXX_STRING& colName)
{
  return this->getAsciiStream(this->findColumn(colName));
}


ODBCXX_STREAM* ResultSet::getBinaryStream(int idx)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
 // CHECK_INSERT_ROW;

  DataHandler* dh=rowset_->getColumn(idx);
  ODBCXX_STREAM* s=dh->getStream();
  if(s==NULL && location_!=INSERT_ROW) {
    s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataStream(this,hstmt_,idx,SQL_C_BINARY,
                     dh->dataStatus_[dh->currentRow_]);
    dh->setStream(s);
  }
 //  else
//  {	dh->setNull();}
 lastWasNull_=dh->isNull();
  return s;
}

ODBCXX_STREAM* ResultSet::getBinaryStream(const ODBCXX_STRING& colName)
{
  return this->getBinaryStream(this->findColumn(colName));
}



void ResultSet::updateAsciiStream(int idx, ODBCXX_STREAM* s, int len)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
  DataHandler* dh=rowset_->getColumn(idx);
  dh->setStream(s,len);
}

void ResultSet::updateAsciiStream(const ODBCXX_STRING& colName,
                                  ODBCXX_STREAM* s, int len)
{
  this->updateAsciiStream(findColumn(colName),s,len);
}


void ResultSet::updateBinaryStream(int idx, ODBCXX_STREAM* s, int len)
{
  CHECK_COL(idx);
  CHECK_LOCATION;
  DataHandler* dh=rowset_->getColumn(idx);
  dh->setStream(s,len);
}

void ResultSet::updateBinaryStream(const ODBCXX_STRING& colName,
                                   ODBCXX_STREAM* s, int len)
{
  this->updateBinaryStream(findColumn(colName),s,len);
}
