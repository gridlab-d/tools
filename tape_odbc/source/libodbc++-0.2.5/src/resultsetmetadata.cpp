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

#include <odbc++/resultsetmetadata.h>
#include <odbc++/resultset.h>
#include "driverinfo.h"

#if FALSE
#include "/DB2/SQLLIB/include/sqlcli.h"
#else
/* SQL extended data types */
#define  SQL_GRAPHIC            -95
#define  SQL_VARGRAPHIC         -96
#define  SQL_LONGVARGRAPHIC     -97
#define  SQL_BLOB               -98
#define  SQL_CLOB               -99
#define  SQL_DBCLOB             -350
#define  SQL_DATALINK           -400
#define  SQL_USER_DEFINED_TYPE  -450
#endif
using namespace odbc;
using namespace std;

// ---------------------------------------
ResultSetMetaData::ResultSetMetaData(ResultSet* rs)
  :resultSet_(rs),
   needsGetData_(false)
{
  this->_fetchColumnInfo();
}


//private
int ResultSetMetaData::_getNumericAttribute(unsigned int col,
					   SQLUSMALLINT attr)
{
  SQLLEN res=0;
  SQLRETURN r=
    ODBC3_C(SQLColAttribute,SQLColAttributes)(resultSet_->hstmt_,
					      (SQLUSMALLINT)col,
					      attr,
					      0,
					      0,
					      0,
					      &res);
  resultSet_->_checkStmtError(resultSet_->hstmt_,
			      r,ODBCXX_STRING_CONST("Error fetching numeric attribute"));
  return (int)res;
}


//private
ODBCXX_STRING
ResultSetMetaData::_getStringAttribute(unsigned int col,
				       SQLUSMALLINT attr, unsigned int maxlen)
{
  ODBCXX_CHAR_TYPE* buf=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[maxlen+1];
  odbc::Deleter<ODBCXX_CHAR_TYPE> _buf(buf,true);
  buf[maxlen]=0;

  SQLLEN res=0;
  SQLSMALLINT len=0;

  SQLRETURN r=
    ODBC3_C(SQLColAttribute,SQLColAttributes)(resultSet_->hstmt_,
					      (SQLUSMALLINT)col,
					      attr,
					      (ODBCXX_SQLCHAR*)buf,
					      (SQLSMALLINT)maxlen,
					      &len,
					      &res);
  resultSet_->_checkStmtError(resultSet_->hstmt_,
			      r,ODBCXX_STRING_CONST("Error fetching string attribute"));
  return ODBCXX_STRING_C(buf);
}

// 
int REMAP_DATATYPE(int type)
{
	int retVal = type;
   switch(retVal)
   {
   case SQL_GRAPHIC: //    -95
       retVal = Types::BINARY;
       break;
   case SQL_VARGRAPHIC: // Image         -96
       retVal = Types::VARBINARY;
       break;
//    case SQL_DATALINK: //         -400
// ???
//    case SQL_USER_DEFINED_TYPE:    // -450
// Should be treated as a BLOB, though it is User defined
   case SQL_LONGVARGRAPHIC: // -97
   case SQL_BLOB:    // -98
       retVal = Types::LONGVARBINARY;
       break;
   case SQL_CLOB:    // -99
       retVal = Types::LONGVARCHAR ;
       break;
   case SQL_DBCLOB:    // Double Byte CLOB -350
       retVal = Types::WLONGVARCHAR;
       break;

   }
   return retVal;
}

// ----------------------------------------------------

#ifdef ODBCXX_USE_INDEXED_METADATA_COLNAMES

/*
	bool operator()(const _Ty& _Left, const _Ty& _Right) const
		{	// apply operator< to operands
		return (_Left < _Right);
		}
*/
bool caseinsesnless::operator()(const ODBCXX_STRING _Left, const ODBCXX_STRING _Right) const
{
	bool retVal = false;
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
       (ODBCXX_STRING_CSTR(_Left),
        ODBCXX_STRING_CSTR(_Right)) < 0) 
	{ retVal = true;  }
	return retVal;
}
int ResultSetMetaData::findColumn(const ODBCXX_STRING& colName)
{
	int retVal = -1;
	std::map<const ODBCXX_STRING, int, CaseInsesitiveLess>::iterator iter =
		colNameIndex_.find(colName);
	if(iter != colNameIndex_.end())
	{	retVal = iter->second;}
	return retVal;
}

#endif // ODBCXX_USE_INDEXED_METADATA_COLNAMES

//private
void ResultSetMetaData::_fetchColumnInfo()
{
  //first argument is ignored
  numCols_=this->_getNumericAttribute
    (1,ODBC3_DC(SQL_DESC_COUNT,SQL_COLUMN_COUNT));

  for(int i=1; i<=numCols_; i++) {
    colNames_.push_back(this->_getStringAttribute
			(i,ODBC3_DC(SQL_DESC_NAME,SQL_COLUMN_NAME)));
#ifdef ODBCXX_USE_INDEXED_METADATA_COLNAMES
	colNameIndex_.insert(make_pair(colNames_[i-1], i));
#endif // ODBCXX_USE_INDEXED_METADATA_COLNAMES
//    int colType=this->_getNumericAttribute
//      (i,ODBC3_DC(SQL_DESC_CONCISE_TYPE,SQL_COLUMN_TYPE));
    int colType=REMAP_DATATYPE(this->_getNumericAttribute
      (i,ODBC3_DC(SQL_DESC_CONCISE_TYPE,SQL_COLUMN_TYPE)));

    colTypes_.push_back(colType);

    //remember if we saw any column that needs GetData
    if(colType==Types::LONGVARCHAR || colType==Types::LONGVARBINARY) {
      needsGetData_=true;
    }

    colPrecisions_.push_back
      (this->_getNumericAttribute
       (i,ODBC3_DC(SQL_DESC_PRECISION,SQL_COLUMN_PRECISION)));

    colScales_.push_back(this->_getNumericAttribute
			 (i,ODBC3_DC(SQL_DESC_SCALE,SQL_COLUMN_SCALE)));

#if ODBCVER >= 0x0300
    if(this->_getDriverInfo()->getMajorVersion()>=3) {
    // ODBC3 does not return character maxlength with SQL_DESC_PRECISION
    // so we need this as well.
      colLengths_.push_back(this->_getNumericAttribute(i,SQL_DESC_LENGTH));
    }
#endif
  }
}

#define CHECK_COL(x) 					\
do {							\
  if(x<1 || x>numCols_) {				\
    throw SQLException					\
      (ODBCXX_STRING_CONST("Column index out of bounds"), ODBCXX_STRING_CONST("42S22"));			\
  } 							\
} while(false)

int ResultSetMetaData::getColumnCount() const
{
  return numCols_;
}

const ODBCXX_STRING& ResultSetMetaData::getColumnName(int col) const
{
  CHECK_COL(col);
  return colNames_[col-1];
}

int ResultSetMetaData::getColumnType(int col) const
{
  CHECK_COL(col);
  return colTypes_[col-1];
}

int ResultSetMetaData::getPrecision(int col) const
{
  CHECK_COL(col);
  return colPrecisions_[col-1];
}

int ResultSetMetaData::getScale(int col) const
{
  CHECK_COL(col);
  return colScales_[col-1];
}


int ResultSetMetaData::getColumnDisplaySize(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute(col,SQL_COLUMN_DISPLAY_SIZE);
}


ODBCXX_STRING ResultSetMetaData::getCatalogName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_CATALOG_NAME,SQL_COLUMN_QUALIFIER_NAME));
}

ODBCXX_STRING ResultSetMetaData::getColumnLabel(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_LABEL,SQL_COLUMN_LABEL));
}

ODBCXX_STRING ResultSetMetaData::getColumnTypeName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_TYPE_NAME,SQL_COLUMN_TYPE_NAME));
}


ODBCXX_STRING ResultSetMetaData::getSchemaName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_SCHEMA_NAME,SQL_COLUMN_OWNER_NAME));
}

ODBCXX_STRING ResultSetMetaData::getTableName(int col)
{
  CHECK_COL(col);
  return this->_getStringAttribute
    (col,ODBC3_DC(SQL_DESC_TABLE_NAME,SQL_COLUMN_TABLE_NAME));
}

bool ResultSetMetaData::isAutoIncrement(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_AUTO_UNIQUE_VALUE,
		  SQL_COLUMN_AUTO_INCREMENT))!=SQL_FALSE;
}

bool ResultSetMetaData::isCaseSensitive(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_CASE_SENSITIVE,SQL_COLUMN_CASE_SENSITIVE))!=SQL_FALSE;
}

bool ResultSetMetaData::isCurrency(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_FIXED_PREC_SCALE,
		  SQL_COLUMN_MONEY))!=SQL_FALSE;
}

bool ResultSetMetaData::isDefinitelyWritable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))==SQL_ATTR_WRITE;
}

int ResultSetMetaData::isNullable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_NULLABLE,SQL_COLUMN_NULLABLE));
}

bool ResultSetMetaData::isReadOnly(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))==SQL_ATTR_READONLY;
}

bool ResultSetMetaData::isSearchable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_SEARCHABLE,SQL_COLUMN_SEARCHABLE))!=SQL_UNSEARCHABLE;
}

bool ResultSetMetaData::isSigned(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UNSIGNED,SQL_COLUMN_UNSIGNED))==SQL_FALSE;
}

bool ResultSetMetaData::isWritable(int col)
{
  CHECK_COL(col);
  return this->_getNumericAttribute
    (col,ODBC3_DC(SQL_DESC_UPDATABLE,SQL_COLUMN_UPDATABLE))!=SQL_ATTR_READONLY;
}


