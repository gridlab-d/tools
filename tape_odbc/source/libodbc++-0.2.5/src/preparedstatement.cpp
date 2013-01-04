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

/*
  We have three cases to hadle here:
  1. The driver supports SQLNumParams and SQLDescribeParam and gives correct
  types for all parameters.
  2. The driver supports SQLNumParams and pretends to support SQLDescribeParam
  but actually says all parameters are VARCHAR(255)
  3. The driver doesn't support SQLNumParams and SQLDescribeParam. In this case
  we have to be able to dynamically add parameters.
*/

#include <odbc++/preparedstatement.h>
#include "datahandler.h"
#include "driverinfo.h"
#include "dtconv.h"

#if defined(ODBCXX_QT)
# include <qiodevice.h>
#endif

using namespace odbc;
using namespace std;

#if !defined(SQL_TIMESTAMP_LEN)
# define SQL_TIMESTAMP_LEN 19
#endif

#if !defined(SQL_DATE_LEN)
# define SQL_DATE_LEN 10
#endif

#if !defined(SQL_TIME_LEN)
# define SQL_TIME_LEN 8
#endif

PreparedStatement::PreparedStatement(Connection* con,
				     SQLHSTMT hstmt,
				     const ODBCXX_STRING& sql,
				     int resultSetType,
				     int resultSetConcurrency,
				     int defaultDirection)
  :Statement(con,hstmt,resultSetType,resultSetConcurrency),
   sql_(sql),
   rowset_(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) Rowset(1,ODBC3_DC(true,false))), //always one row for now
   numParams_(0),
   defaultDirection_(defaultDirection),
   paramsBound_(false)
{
  this->_prepare();
  this->_setupParams();
}

PreparedStatement::~PreparedStatement()
{
  if(paramsBound_) {
    this->_unbindParams();
  }

  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) rowset_;
}

void PreparedStatement::_prepare()
{
  SQLRETURN r=SQLPrepare(hstmt_,
			 (ODBCXX_SQLCHAR*)ODBCXX_STRING_DATA(sql_),
			 ODBCXX_STRING_LEN(sql_));

  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Error preparing ")+sql_;
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CSTR(msg), ODBCXX_STRING_CONST("HY007"));
}

void PreparedStatement::_checkParam(int idx,
				    int* allowed, int numAllowed,
				    int defPrec, int defScale)
{
    // we put a restriction when using drivers that don't support
    // SQLNumParams: All parameters have to be set in increasing order,
    // starting from 1

  if(idx<=0 || idx>numParams_+1) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: PreparedStatement: parameter index ")+
       intToString(idx)+ODBCXX_STRING_CONST(" out of bounds"), ODBCXX_STRING_CONST("S1093"));
  }
  
  assert(allowed!=NULL && numAllowed>0);

  if(numParams_<(unsigned int)idx) {

    if(paramsBound_) {
      this->_unbindParams();
    }

    // just add a column
    rowset_->addColumn(allowed[0],defPrec,defScale);
    directions_.push_back(defaultDirection_);
    numParams_++;

    // nothing more to do here
    return;
  }

  assert(idx<=numParams_ && idx>0);

  // we have a (not yet implemented) batch of parameters, and we
  // can't change the type of datahandlers except on the first row
  if(rowset_->getCurrentRow()>0) {
    return;
  }

  bool replace=true;
  DataHandler* dh=rowset_->getColumn(idx);

  for(int i=0; i<numAllowed; i++) {
    if(dh->getSQLType()==allowed[i]) {
      replace=false;
      break;
    }
  }

  if(replace) {
    if(paramsBound_) {
      // we are changing a buffer address, unbind 
      // the parameters
      this->_unbindParams();
    }
    rowset_->replaceColumn(idx,allowed[0],defPrec,defScale);
  }
}


void PreparedStatement::_setupParams()
{
  if(!this->_getDriverInfo()->supportsFunction(SQL_API_SQLNUMPARAMS)) {
    return;
  }

  SQLSMALLINT np;
  SQLRETURN r=SQLNumParams(hstmt_,&np);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching number of parameters"));
  numParams_=np;
  
  SQLSMALLINT sqlType;
  SQLULEN prec;
  SQLSMALLINT scale;
  SQLSMALLINT nullable;

  if(this->_getDriverInfo()->supportsFunction(SQL_API_SQLDESCRIBEPARAM)) {
   
    for(size_t i=0; i<numParams_; i++) {
      r=SQLDescribeParam(hstmt_,
			 i+1,
			 &sqlType,
			 &prec,
			 &scale,
			 &nullable);

      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error obtaining parameter information"));

      // fix for drivers that actually return 0 for precision
      if(prec==0 && scale==0) {
	prec=DataHandler::defaultPrecisionFor(sqlType);
      }
      
      rowset_->addColumn(sqlType,prec,scale);
      directions_.push_back(defaultDirection_);
    }
  } else {
    // default all parameters to VARCHAR(255)
    for(size_t i=0; i<numParams_; i++) {
      rowset_->addColumn(Types::VARCHAR,255,0);
      directions_.push_back(defaultDirection_);
    }
  }
}


void PreparedStatement::_bindParams()
{
  SQLRETURN r;
  for(size_t i=1; i<=numParams_; i++) {
    DataHandler* dh=rowset_->getColumn(i);

    //simple bind
    if(!dh->isStreamed_) {
      r=SQLBindParameter(hstmt_,
			 (SQLUSMALLINT)i,
			 (SQLSMALLINT)directions_[i-1],
			 (SQLSMALLINT)dh->cType_,
			 (SQLSMALLINT)dh->sqlType_,
			 (SQLUINTEGER)dh->precision_,
			 (SQLSMALLINT)dh->scale_,
			 (ODBCXX_SQLCHAR*)dh->data(),
			 dh->bufferSize_,
			 dh->dataStatus_);

    } else {

#if !defined(WIN32)
      SQLUINTEGER precision=0;
#else
      SQLUINTEGER precision=dh->dataStatus_[i-1];
      if(precision!=SQL_NULL_DATA) {
	// assume SQL_LEN_DATA_AT_EXEC(SQL_LEN_DATA_AT_EXEC(x))==x
	precision=SQL_LEN_DATA_AT_EXEC(precision);
      } else {
	precision=0;
      }
#endif
      //we send in dh->dataStatus_, as it contains
      //SQL_LEN_DATA_AT_EXEC(size) after setStream, or SQL_NULL_DATA
      
      //We need to store the column index, which is a transient integer,
      //as a parameter value, which is a pointer.  We store the index
      //in a set and use a pointer to the stored value.
      std::set<SQLUINTEGER>::iterator indexIter( columnIndicies_.insert(i).first );
      SQLUINTEGER *indexPtr( const_cast<SQLUINTEGER *>(&(*indexIter)) );
      
      r=SQLBindParameter(hstmt_,
			 (SQLUSMALLINT)i,
			 (SQLSMALLINT)directions_[i-1],
			 (SQLSMALLINT)dh->cType_,
			 (SQLSMALLINT)dh->sqlType_,
			 precision,
			 0, //same here
			 static_cast<SQLPOINTER>(indexPtr), //our column index
			 0, //doesn't apply
			 dh->dataStatus_);
    }
    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error binding parameter"));
  }

  paramsBound_=true;
}

void PreparedStatement::_unbindParams()
{
  SQLRETURN r=SQLFreeStmt(hstmt_,SQL_RESET_PARAMS);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error unbinding parameters"));
  
  //notify our parameters (should this go into execute()?)
  for(size_t i=1; i<=numParams_; i++) {
    rowset_->getColumn(i)->afterUpdate();
  }
  
  paramsBound_=false;
}



bool PreparedStatement::execute()
{
#if 0
  unsigned int nc=rowset_->getColumns();
  ODBCXX_COUT << ODBCXX_STRING_CONST("Entering PreparedStatement::execute()") << endl
       << ODBCXX_STRING_CONST("Parameters: ") << numParams_
       << ODBCXX_STRING_CONST(", rowset columns: ") << nc << endl;

  for(int i=1; i<=nc; i++) {
    DataHandler* dh=rowset_->getColumn(i);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Parameter ") << i << ODBCXX_STRING_CONST(":") << endl;
    ODBCXX_COUT << ODBCXX_STRING_CONST("SQLType   : ") << dh->getSQLType() << endl
	 << ODBCXX_STRING_CONST("Value     : ")
    << (dh->isNull()?ODBCXX_STRING_CONST("<NULL>"):dh->getString()) << endl
	 << ODBCXX_STRING_CONST("Direction : ") << directions_[i-1] << endl
	 << endl;
  }
#endif

  this->_beforeExecute();

  if(!paramsBound_) {
    this->_bindParams();
  }

  SQLRETURN r=SQLExecute(hstmt_);

  // remember this for getUpdateCount()
  lastExecute_=r;

  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Error executing \"")+sql_+ODBCXX_STRING_CONST("\"");
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CSTR(msg));

  //the following should maybe join with ResultSet::updateRow/insertRow in
  //some way

  if(r==SQL_NEED_DATA) {
    ODBCXX_CHAR_TYPE buf[PUTDATA_CHUNK_SIZE];
    
    while(r==SQL_NEED_DATA) {
      SQLPOINTER currentCol;
      r=SQLParamData(hstmt_,&currentCol);
      this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("SQLParamData failure"));
      if(r==SQL_NEED_DATA) {
	DataHandler* dh=rowset_->getColumn(*static_cast<SQLUINTEGER *>(currentCol));

	assert(dh->isStreamed_);
	
	ODBCXX_STREAM* s=dh->getStream();

	assert(s!=NULL);

	int b;

	// we don't want to write more data than set with setXXXStream

	// we trust the SQL_LEN_DATA_AT_EXEC macro to work both ways
	int streamSize=SQL_LEN_DATA_AT_EXEC(dh->getDataStatus());
	int charsLeft=streamSize;

	if(streamSize>0) {
	  while(charsLeft>0 &&
		(b=readStream(s,buf,min(charsLeft,PUTDATA_CHUNK_SIZE)))>0) {
	    charsLeft-=b;
	    assert(charsLeft>=0);
	    SQLRETURN rPD=SQLPutData(hstmt_,(SQLPOINTER)buf,b*sizeof(ODBCXX_CHAR_TYPE));
	    this->_checkStmtError(hstmt_,rPD,ODBCXX_STRING_CONST("SQLPutData failure"));
	  }
	}
	
	if(charsLeft==streamSize) {
	  // SQLPutData has not been called, call it with zero size
	  SQLRETURN rPD=SQLPutData(hstmt_,(SQLPOINTER)buf,0);
	  this->_checkStmtError(hstmt_,rPD,ODBCXX_STRING_CONST("SQLPutData(0) failure"));
	}
      }
    }
  }

  this->_afterExecute();

  return this->_checkForResults();
}


ResultSet* PreparedStatement::executeQuery()
{
  this->execute();
  return this->getResultSet();
}


int PreparedStatement::executeUpdate()
{
  this->execute();
  return this->getUpdateCount();
}


void PreparedStatement::clearParameters()
{
  if(paramsBound_) {
    this->_unbindParams();
  }

  for(size_t i=1; i<=numParams_; i++) {
    rowset_->getColumn(i)->setNull();
  }
}

void PreparedStatement::setNull(int idx, int sqlType)
{
  int defPrec=DataHandler::defaultPrecisionFor(sqlType);
  this->_checkParam(idx,
		    &sqlType,1,
		    defPrec,0);
  rowset_->getColumn(idx)->setNull();
}


// ALLOWED[0] must be the default type!
#define IMPLEMENT_SET(TYPE,FUNCSUFFIX,ALLOWED,DEFPREC)		\
void PreparedStatement::set##FUNCSUFFIX(int idx, TYPE val)	\
{								\
  int allowed[]=ALLOWED;					\
  this->_checkParam(idx,					\
		    allowed,sizeof(allowed)/sizeof(int),	\
		    DEFPREC,0);					\
  rowset_->getColumn(idx)->set##FUNCSUFFIX(val);		\
}

#define A_1(t) {t}
#define A_2(t1,t2) {t1,t2}
#define A_3(t1,t2,t3) {t1,t2,t3}
#define A_4(t1,t2,t3,t4) {t1,t2,t3,t4}


IMPLEMENT_SET(double,Double,
	      A_1(Types::DOUBLE),
	      0);

IMPLEMENT_SET(bool,Boolean,
	      A_2(Types::BIT,Types::TINYINT),
	      0);

IMPLEMENT_SET(signed char,Byte,
	      A_1(Types::TINYINT),
	      0);



IMPLEMENT_SET(float, Float,
	      A_3(Types::REAL,Types::FLOAT,Types::DOUBLE),
	      0);

IMPLEMENT_SET(int,Int,
	      A_1(Types::INTEGER),
	      0);

IMPLEMENT_SET(Long,Long,
	      A_3(Types::BIGINT,Types::NUMERIC,Types::DECIMAL),
	      0);

IMPLEMENT_SET(short,Short,
	      A_2(Types::SMALLINT,Types::INTEGER),
	      0);

#if defined(ODBCXX_HAVE_SQLUCODE_H)
IMPLEMENT_SET(const ODBCXX_STRING&,String,
	      A_4(Types::VARCHAR,Types::CHAR,Types::WVARCHAR,Types::WCHAR),
	      255);
#else
IMPLEMENT_SET(const ODBCXX_STRING&,String,
	      A_2(Types::VARCHAR,Types::CHAR),
	      255);
#endif

IMPLEMENT_SET(const Date&,Date,
	      A_1(Types::DATE),
	      SQL_DATE_LEN);

IMPLEMENT_SET(const Time&,Time,
	      A_1(Types::TIME),
	      SQL_TIME_LEN);

IMPLEMENT_SET(const Timestamp&, Timestamp,
	      A_1(Types::TIMESTAMP),
	      SQL_TIMESTAMP_LEN);

IMPLEMENT_SET(const ODBCXX_BYTES&, Bytes, 
	      A_2(Types::VARBINARY,Types::BINARY),
	      0);

void PreparedStatement::setAsciiStream(int idx, ODBCXX_STREAM* s, int len)
{
  int allowed=Types::LONGVARCHAR;

  this->_checkParam(idx,&allowed, 1,0,0);
  rowset_->getColumn(idx)->setStream(s,len);
}

void PreparedStatement::setBinaryStream(int idx, ODBCXX_STREAM* s, int len)
{
  int allowed=Types::LONGVARBINARY;

  this->_checkParam(idx,&allowed,1,0,0);
  rowset_->getColumn(idx)->setStream(s,len);
}

#ifdef ODBCXX_HAVE_STRUCT_GUID
void PreparedStatement::setGuid(int idx, odbc::Guid guid) {
	int allowed=Types::GUID;
	this->_checkParam(idx, &allowed,1,0,0);
	rowset_->getColumn(idx)->setGuid(guid);
}
#endif
