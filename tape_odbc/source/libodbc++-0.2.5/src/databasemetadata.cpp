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

#include <odbc++/databasemetadata.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/statement.h>

#include "dtconv.h"
#include "driverinfo.h"

using namespace odbc;
using namespace std;


#define FETCH_STEP 64

namespace odbc {

  // returns the actual ODBC cursor type this datasource would
  // use for a given ResultSet type
inline int getODBCCursorTypeFor(int rsType, const DriverInfo* di)
{
  int r;
  switch(rsType) {
  case ResultSet::TYPE_FORWARD_ONLY:
    r=SQL_CURSOR_FORWARD_ONLY;
    break;
  case ResultSet::TYPE_SCROLL_INSENSITIVE:
    r=SQL_CURSOR_STATIC;
    break;
  case ResultSet::TYPE_SCROLL_SENSITIVE:
    if(di->getCursorMask()&SQL_SO_DYNAMIC) {
      r=SQL_CURSOR_DYNAMIC;
    } else {
      r=SQL_CURSOR_KEYSET_DRIVEN;
    }
    break;
  default:
    throw SQLException(ODBCXX_STRING_CONST("[libodbc++]: Invalid ResultSet type ")+intToString(rsType), ODBCXX_STRING_CONST("S1009"));
  }

  return r;
}

#if ODBCVER >= 0x0300
inline int getCursorAttributes1For(int odbcType)
{
  int infoType;
  switch(odbcType) {
  case SQL_CURSOR_FORWARD_ONLY:
    infoType=SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1;
    break;
  case SQL_CURSOR_STATIC:
    infoType=SQL_STATIC_CURSOR_ATTRIBUTES1;
    break;
  case SQL_CURSOR_KEYSET_DRIVEN:
    infoType=SQL_KEYSET_CURSOR_ATTRIBUTES1;
    break;
  case SQL_CURSOR_DYNAMIC:
  default:
    infoType=SQL_DYNAMIC_CURSOR_ATTRIBUTES1;
    break;
  }
  return infoType;
}

inline int getCursorAttributes2For(int odbcType)
{
  int infoType;
  switch(odbcType) {
  case SQL_CURSOR_FORWARD_ONLY:
    infoType=SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2;
    break;
  case SQL_CURSOR_STATIC:
    infoType=SQL_STATIC_CURSOR_ATTRIBUTES2;
    break;
  case SQL_CURSOR_KEYSET_DRIVEN:
    infoType=SQL_KEYSET_CURSOR_ATTRIBUTES2;
    break;
  case SQL_CURSOR_DYNAMIC:
  default:
    infoType=SQL_DYNAMIC_CURSOR_ATTRIBUTES2;
    break;
  }
  return infoType;
}

#endif // ODBCVER >= 0x0300

// for _ownXXXAreVisible
enum {
  INSERTS,UPDATES,DELETES
};

}; // namespace odbc


DatabaseMetaData::DatabaseMetaData(Connection* c)
  :connection_(c)
{
}

DatabaseMetaData::~DatabaseMetaData()
{
}


SQLUSMALLINT DatabaseMetaData::_getNumeric16(int what)
{
  SQLUSMALLINT res;
  SQLSMALLINT t;
  SQLRETURN r=SQLGetInfo(connection_->hdbc_,
			 (SQLUSMALLINT)what,
			 &res,
			 sizeof(res), //ignored, but what the heck..
			 &t);
  connection_->_checkConError(connection_->hdbc_,
			      r,ODBCXX_STRING_CONST("Error fetching information"));

  return res;
}

SQLUINTEGER DatabaseMetaData::_getNumeric32(int what)
{
  SQLUINTEGER res;
  SQLSMALLINT t;
  SQLRETURN r=SQLGetInfo(connection_->hdbc_,
			 (SQLUSMALLINT)what,
			 &res,
			 sizeof(res), //ignored, but what the heck..
			 &t);
  connection_->_checkConError(connection_->hdbc_,r,ODBCXX_STRING_CONST("Error fetching information"));

  return res;
}


//private
ODBCXX_STRING DatabaseMetaData::_getStringInfo(int what)
{
  ODBCXX_STRING res;
  SQLSMALLINT len1;
  SQLSMALLINT len2=FETCH_STEP;
  ODBCXX_CHAR_TYPE* buf=NULL;

  // ODBCXX_CERR << ODBCXX_STRING_CONST("Entering _getStringInfo()") << endl;

  do {
    len1=len2;

    buf=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[len1+1];

    //    ODBCXX_CERR << ODBCXX_STRING_CONST("Calling SQLGetInfo: len1=")
    //                << len1 << ODBCXX_STRING_CONST(", len2=") << len2 << endl;

    SQLRETURN r=SQLGetInfo(connection_->hdbc_,
			   (SQLUSMALLINT)what,
			   buf,
			   len1+1,
			   &len2);
    try {
      connection_->_checkConError(connection_->hdbc_,r,ODBCXX_STRING_CONST("Error fetching information"));
    } catch(...) {
      delete[] buf;
      throw;
    }
  } while(len2>len1);

  // ODBCXX_CERR << ODBCXX_STRING_CONST("Exiting _getStringInfo()") << endl;

  res=ODBCXX_STRING_C(buf);
  delete[] buf;
  return res;
}


#if ODBCVER >= 0x0300

SQLUINTEGER DatabaseMetaData::_getAllCursorAttributes1()
{
  SQLUINTEGER r=0;
  int cm=this->_getDriverInfo()->getCursorMask();
  if(cm&SQL_SO_FORWARD_ONLY) {
    r|=this->_getNumeric32(SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1);
  }
  if(cm&SQL_SO_STATIC) {
    r|=this->_getNumeric32(SQL_STATIC_CURSOR_ATTRIBUTES1);
  }
  if(cm&SQL_SO_KEYSET_DRIVEN) {
    r|=this->_getNumeric32(SQL_KEYSET_CURSOR_ATTRIBUTES1);
  }
  if(cm&SQL_SO_DYNAMIC) {
    r|=this->_getNumeric32(SQL_DYNAMIC_CURSOR_ATTRIBUTES1);
  }
  return r;
}

#endif


bool DatabaseMetaData::supportsBatchUpdates()
{
  // we don't, yet
  return false;
}

bool DatabaseMetaData::supportsIntegrityEnhancementFacility()
{
  return this->_getStringInfo(SQL_ODBC_SQL_OPT_IEF)==ODBCXX_STRING_CONST("Y");
}


bool DatabaseMetaData::supportsPositionedDelete()
{
  // with ODBC 2, we can simply check SQL_POSITIONED_STATEMENTS
  // for ODBC 3, we have to check if any cursor type supports this

#if ODBCVER >= 0x0300
  if(this->_getDriverInfo()->getMajorVersion()<3) {
#endif

    return (this->_getNumeric32
	    (SQL_POSITIONED_STATEMENTS)&SQL_PS_POSITIONED_DELETE)!=0;

#if ODBCVER >= 0x0300
  }

  // the ODBC 3 way
  return (this->_getAllCursorAttributes1()&SQL_CA1_POSITIONED_DELETE)!=0;

#endif
}

bool DatabaseMetaData::supportsPositionedUpdate()
{
#if ODBCVER >= 0x0300
  if(this->_getDriverInfo()->getMajorVersion()<3) {
#endif

    return (this->_getNumeric32
	    (SQL_POSITIONED_STATEMENTS)&SQL_PS_POSITIONED_UPDATE)!=0;

#if ODBCVER >= 0x0300
  }

  // the ODBC 3 way
  return (this->_getAllCursorAttributes1()&SQL_CA1_POSITIONED_UPDATE)!=0;

#endif
}


bool DatabaseMetaData::supportsSelectForUpdate()
{
#if ODBCVER >= 0x0300
  if(this->_getDriverInfo()->getMajorVersion()<3) {
#endif

    return (this->_getNumeric32
	    (SQL_POSITIONED_STATEMENTS)&SQL_PS_SELECT_FOR_UPDATE)!=0;

#if ODBCVER >= 0x0300
  }

  // the ODBC 3 way
  return (this->_getAllCursorAttributes1()&SQL_CA1_SELECT_FOR_UPDATE)!=0;

#endif
}



bool DatabaseMetaData::supportsTransactions()
{
  return this->_getNumeric16(SQL_TXN_CAPABLE)!=SQL_TC_NONE;
}

bool DatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions()
{
  return this->_getNumeric16
    (SQL_TXN_CAPABLE)==SQL_TC_ALL;
}

bool DatabaseMetaData::supportsDataManipulationTransactionsOnly()
{
  return this->_getNumeric16
    (SQL_TXN_CAPABLE)==SQL_TC_DML;
}

bool DatabaseMetaData::dataDefinitionCausesTransactionCommit()
{
  return this->_getNumeric16
    (SQL_TXN_CAPABLE)==SQL_TC_DDL_COMMIT;
}

bool DatabaseMetaData::dataDefinitionIgnoredInTransactions()
{
  return this->_getNumeric16
    (SQL_TXN_CAPABLE)==SQL_TC_DDL_IGNORE;
}



ODBCXX_STRING DatabaseMetaData::getDatabaseProductName()
{
  return this->_getStringInfo(SQL_DBMS_NAME);
}

ODBCXX_STRING DatabaseMetaData::getDatabaseProductVersion()
{
  return this->_getStringInfo(SQL_DBMS_VER);
}

ODBCXX_STRING DatabaseMetaData::getDriverName()
{
  return this->_getStringInfo(SQL_DRIVER_NAME);
}


ODBCXX_STRING DatabaseMetaData::getDriverVersion()
{
  return this->_getStringInfo(SQL_DRIVER_VER);
}

int DatabaseMetaData::getDriverMajorVersion()
{
  ODBCXX_STRING s=this->_getStringInfo(SQL_DRIVER_ODBC_VER);
  if(s.length()==5) {
    return stringToInt(
#if defined(ODBCXX_QT)
		       s.left(2)
#else
		       s.substr(0,2)
#endif
		       );
  }
  throw SQLException
    (ODBCXX_STRING_CONST("[libodbc++]: Invalid ODBC version string received from driver: ")+s, SQLException::scDEFSQLSTATE);

  ODBCXX_DUMMY_RETURN(0);
}

int DatabaseMetaData::getDriverMinorVersion()
{
  ODBCXX_STRING s=this->_getStringInfo(SQL_DRIVER_ODBC_VER);
  if(s.length()==5) {
    return stringToInt(
#if defined(ODBCXX_QT)
		       s.mid(3,2)
#else
		       s.substr(3,2)
#endif
		       );
  }
  throw SQLException
    (ODBCXX_STRING_CONST("[libodbc++]: Invalid ODBC version string received from driver: ")+s, SQLException::scDEFSQLSTATE);
  ODBCXX_DUMMY_RETURN(0);
}

ODBCXX_STRING DatabaseMetaData::getIdentifierQuoteString()
{
  return this->_getStringInfo(SQL_IDENTIFIER_QUOTE_CHAR);
}


ODBCXX_STRING DatabaseMetaData::getCatalogTerm()
{
  return this->_getStringInfo(ODBC3_C(SQL_CATALOG_TERM,SQL_QUALIFIER_TERM));
}

ODBCXX_STRING DatabaseMetaData::getSchemaTerm()
{
  return this->_getStringInfo(ODBC3_C(SQL_SCHEMA_TERM,SQL_OWNER_TERM));
}

ODBCXX_STRING DatabaseMetaData::getTableTerm()
{
  return this->_getStringInfo(SQL_TABLE_TERM);
}

ODBCXX_STRING DatabaseMetaData::getProcedureTerm()
{
  return this->_getStringInfo(SQL_PROCEDURE_TERM);
}

ODBCXX_STRING DatabaseMetaData::getUserName()
{
  return this->_getStringInfo(SQL_USER_NAME);
}

ODBCXX_STRING DatabaseMetaData::getCatalogSeparator()
{
  return this->_getStringInfo(ODBC3_C(SQL_CATALOG_NAME_SEPARATOR,
				      SQL_QUALIFIER_NAME_SEPARATOR));
}


bool DatabaseMetaData::isCatalogAtStart()
{
  return this->_getNumeric16(ODBC3_C(SQL_CATALOG_LOCATION,
				     SQL_QUALIFIER_LOCATION))==SQL_QL_START;
}

ODBCXX_STRING DatabaseMetaData::getSQLKeywords()
{
  return this->_getStringInfo(SQL_KEYWORDS);
}



int DatabaseMetaData::getDefaultTransactionIsolation()
{
  SQLUINTEGER r=this->_getNumeric32(SQL_DEFAULT_TXN_ISOLATION);
  switch(r) {
  case SQL_TXN_READ_UNCOMMITTED:
    return Connection::TRANSACTION_READ_UNCOMMITTED;

  case SQL_TXN_READ_COMMITTED:
    return Connection::TRANSACTION_READ_COMMITTED;

  case SQL_TXN_REPEATABLE_READ:
    return Connection::TRANSACTION_REPEATABLE_READ;

  case SQL_TXN_SERIALIZABLE:
#if defined(SQL_TXN_VERSIONING)
  case SQL_TXN_VERSIONING:
#endif
    return Connection::TRANSACTION_SERIALIZABLE;
  }
  return Connection::TRANSACTION_NONE;
}

bool DatabaseMetaData::supportsTransactionIsolationLevel(int lev)
{
  SQLUINTEGER r=this->_getNumeric32(SQL_TXN_ISOLATION_OPTION);

  SQLUINTEGER ret=0;

  switch(lev) {
  case Connection::TRANSACTION_READ_UNCOMMITTED:
    ret=r&SQL_TXN_READ_UNCOMMITTED;
    break;
  case Connection::TRANSACTION_READ_COMMITTED:
    ret=r&SQL_TXN_READ_COMMITTED;
    break;
  case Connection::TRANSACTION_REPEATABLE_READ:
    ret=r&SQL_TXN_REPEATABLE_READ;
    break;
  case Connection::TRANSACTION_SERIALIZABLE:
    ret=(r&SQL_TXN_SERIALIZABLE)
#if defined(SQL_TXN_VERSIONING)
      || (r&SQL_TXN_VERSIONING)
#endif
      ;
    break;
  }

  return ret!=0;
}



bool DatabaseMetaData::supportsOpenCursorsAcrossCommit()
{
  return this->_getNumeric16(SQL_CURSOR_COMMIT_BEHAVIOR)==SQL_CB_PRESERVE;
}

bool DatabaseMetaData::supportsOpenCursorsAcrossRollback()
{
  return this->_getNumeric16(SQL_CURSOR_ROLLBACK_BEHAVIOR)==SQL_CB_PRESERVE;
}


bool DatabaseMetaData::supportsOpenStatementsAcrossCommit()
{
  return this->_getNumeric16(SQL_CURSOR_COMMIT_BEHAVIOR)==SQL_CB_PRESERVE;
}

bool DatabaseMetaData::supportsOpenStatementsAcrossRollback()
{
  return this->_getNumeric16(SQL_CURSOR_ROLLBACK_BEHAVIOR)==SQL_CB_PRESERVE;
}


bool DatabaseMetaData::supportsResultSetType(int type)
{
  const DriverInfo* di=this->_getDriverInfo();

  switch(type) {
  case ResultSet::TYPE_FORWARD_ONLY:
    return di->supportsForwardOnly();
    break;

  case ResultSet::TYPE_SCROLL_INSENSITIVE:
    return di->supportsStatic();
    break;

  case ResultSet::TYPE_SCROLL_SENSITIVE:
    return di->supportsScrollSensitive();
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid ResultSet type ")+
       intToString(type), SQLException::scDEFSQLSTATE);
  }

  ODBCXX_DUMMY_RETURN(false);
}


bool DatabaseMetaData::supportsResultSetConcurrency(int type,
						    int concurrency)
{
  const DriverInfo* di=this->_getDriverInfo();

  if(!this->supportsResultSetType(type)) {
    // no need to bother
    return false;
  }

  int ct;

  switch(type) {
  case ResultSet::TYPE_SCROLL_SENSITIVE:
    ct=di->getScrollSensitive();
    break;

  case ResultSet::TYPE_SCROLL_INSENSITIVE:
    ct=SQL_CURSOR_STATIC;
    break;

  case ResultSet::TYPE_FORWARD_ONLY:
    // forward only cursors are read-only by definition
    return concurrency==ResultSet::CONCUR_READ_ONLY;
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid ResultSet concurrency ")+intToString(type), ODBCXX_STRING_CONST("S1108"));
  }

  switch(concurrency) {
  case ResultSet::CONCUR_READ_ONLY:
    return di->supportsReadOnly(ct);
    break;

  case ResultSet::CONCUR_UPDATABLE:
    return di->supportsUpdatable(ct);
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid ResultSet concurrency ")+intToString(type), ODBCXX_STRING_CONST("S1108"));
  }

  ODBCXX_DUMMY_RETURN(false);
}


bool DatabaseMetaData::nullPlusNonNullIsNull()
{
  return this->_getNumeric16
    (SQL_CONCAT_NULL_BEHAVIOR)==SQL_CB_NULL;
}

bool DatabaseMetaData::supportsColumnAliasing()
{
  return this->_getStringInfo(SQL_COLUMN_ALIAS)==ODBCXX_STRING_CONST("Y");
}


bool DatabaseMetaData::supportsAlterTableWithAddColumn()
{
  return (this->_getNumeric32
	  (SQL_ALTER_TABLE)&SQL_AT_ADD_COLUMN)!=0;
}

bool DatabaseMetaData::supportsAlterTableWithDropColumn()
{
  return (this->_getNumeric32
	  (SQL_ALTER_TABLE)&SQL_AT_DROP_COLUMN)!=0;
}

ODBCXX_STRING DatabaseMetaData::getExtraNameCharacters()
{
  return this->_getStringInfo(SQL_SPECIAL_CHARACTERS);
}

ODBCXX_STRING DatabaseMetaData::getSearchStringEscape()
{
  return this->_getStringInfo(SQL_SEARCH_PATTERN_ESCAPE);
}

ODBCXX_STRING DatabaseMetaData::getNumericFunctions()
{
  static struct {
    int funcId;
    const ODBCXX_CHAR_TYPE* funcName;
  } fmap[] = {
    { SQL_FN_NUM_ABS,      ODBCXX_STRING_CONST("ABS") },
    { SQL_FN_NUM_ACOS,     ODBCXX_STRING_CONST("ACOS") },
    { SQL_FN_NUM_ASIN,     ODBCXX_STRING_CONST("ASIN") },
    { SQL_FN_NUM_ATAN,     ODBCXX_STRING_CONST("ATAN") },
    { SQL_FN_NUM_ATAN2,    ODBCXX_STRING_CONST("ATAN2") },
    { SQL_FN_NUM_CEILING,  ODBCXX_STRING_CONST("CEILING") },
    { SQL_FN_NUM_COS,      ODBCXX_STRING_CONST("COS") },
    { SQL_FN_NUM_COT,      ODBCXX_STRING_CONST("COT") },
    { SQL_FN_NUM_DEGREES,  ODBCXX_STRING_CONST("DEGREES") },
    { SQL_FN_NUM_EXP,      ODBCXX_STRING_CONST("EXP") },
    { SQL_FN_NUM_FLOOR,    ODBCXX_STRING_CONST("FLOOR") },
    { SQL_FN_NUM_LOG,      ODBCXX_STRING_CONST("LOG") },
    { SQL_FN_NUM_LOG10,    ODBCXX_STRING_CONST("LOG10") },
    { SQL_FN_NUM_MOD,      ODBCXX_STRING_CONST("MOD") },
    { SQL_FN_NUM_PI,       ODBCXX_STRING_CONST("PI") },
    { SQL_FN_NUM_POWER,    ODBCXX_STRING_CONST("POWER") },
    { SQL_FN_NUM_RADIANS,  ODBCXX_STRING_CONST("RADIANS") },
    { SQL_FN_NUM_RAND,     ODBCXX_STRING_CONST("RAND") },
    { SQL_FN_NUM_ROUND,    ODBCXX_STRING_CONST("ROUND") },
    { SQL_FN_NUM_SIGN,     ODBCXX_STRING_CONST("SIGN") },
    { SQL_FN_NUM_SIN,      ODBCXX_STRING_CONST("SIN") },
    { SQL_FN_NUM_SQRT,     ODBCXX_STRING_CONST("SQRT") },
    { SQL_FN_NUM_TAN,      ODBCXX_STRING_CONST("TAN") },
    { SQL_FN_NUM_TRUNCATE, ODBCXX_STRING_CONST("TRUNCATE") },
    { 0, NULL }
  };

  SQLUINTEGER r=this->_getNumeric32
    (SQL_NUMERIC_FUNCTIONS);

  ODBCXX_STRING ret=ODBCXX_STRING_CONST("");
  for(int i=0; fmap[i].funcId>0; i++) {
    if(r&(fmap[i].funcId)) {
      if(ret.length()>0) {
	ret+=ODBCXX_STRING_CONST(",");
      }
      ret+=fmap[i].funcName;
    }
  }
  return ret;
}

ODBCXX_STRING DatabaseMetaData::getStringFunctions()
{
  static struct {
    int funcId;
    const ODBCXX_CHAR_TYPE* funcName;
  } fmap[] = {
#if ODBCVER>=0x0300
    { SQL_FN_STR_BIT_LENGTH,       ODBCXX_STRING_CONST("BIT_LENGTH") },
    { SQL_FN_STR_CHAR_LENGTH,      ODBCXX_STRING_CONST("CHAR_LENGTH") },
    { SQL_FN_STR_CHARACTER_LENGTH, ODBCXX_STRING_CONST("CHARACTER_LENGTH") },
    { SQL_FN_STR_OCTET_LENGTH,     ODBCXX_STRING_CONST("OCTET_LENGTH") },
    { SQL_FN_STR_POSITION,         ODBCXX_STRING_CONST("POSITION") },
#endif
    { SQL_FN_STR_ASCII,            ODBCXX_STRING_CONST("ASCII") },
    { SQL_FN_STR_CHAR,             ODBCXX_STRING_CONST("CHAR") },
    { SQL_FN_STR_CONCAT,           ODBCXX_STRING_CONST("CONCAT") },
    { SQL_FN_STR_DIFFERENCE,       ODBCXX_STRING_CONST("DIFFERENCE") },
    { SQL_FN_STR_INSERT,           ODBCXX_STRING_CONST("INSERT") },
    { SQL_FN_STR_LCASE,            ODBCXX_STRING_CONST("LCASE") },
    { SQL_FN_STR_LEFT,             ODBCXX_STRING_CONST("LEFT") },
    { SQL_FN_STR_LENGTH,           ODBCXX_STRING_CONST("LENGTH") },
    { SQL_FN_STR_LOCATE,           ODBCXX_STRING_CONST("LOCATE") },
    { SQL_FN_STR_LOCATE_2,         ODBCXX_STRING_CONST("LOCATE_2") },
    { SQL_FN_STR_LTRIM,            ODBCXX_STRING_CONST("LTRIM") },
    { SQL_FN_STR_REPEAT,           ODBCXX_STRING_CONST("REPEAT") },
    { SQL_FN_STR_REPLACE,          ODBCXX_STRING_CONST("REPLACE") },
    { SQL_FN_STR_RIGHT,            ODBCXX_STRING_CONST("RIGHT") },
    { SQL_FN_STR_RTRIM,            ODBCXX_STRING_CONST("RTRIM") },
    { SQL_FN_STR_SOUNDEX,          ODBCXX_STRING_CONST("SOUNDEX") },
    { SQL_FN_STR_SPACE,            ODBCXX_STRING_CONST("SPACE") },
    { SQL_FN_STR_SUBSTRING,        ODBCXX_STRING_CONST("SUBSTRING") },
    { SQL_FN_STR_UCASE,            ODBCXX_STRING_CONST("UCASE") },
    { 0, NULL }
  };

  SQLUINTEGER r=this->_getNumeric32
    (SQL_STRING_FUNCTIONS);

  ODBCXX_STRING ret=ODBCXX_STRING_CONST("");
  for(int i=0; fmap[i].funcId>0; i++) {
    if(r&(fmap[i].funcId)) {
      if(ret.length()>0) {
	ret+=ODBCXX_STRING_CONST(",");
      }
      ret+=fmap[i].funcName;
    }
  }
  return ret;
}

ODBCXX_STRING DatabaseMetaData::getSystemFunctions()
{
  static struct {
    int funcId;
    const ODBCXX_CHAR_TYPE* funcName;
  } fmap[] = {
    { SQL_FN_SYS_DBNAME,   ODBCXX_STRING_CONST("DBNAME") },
    { SQL_FN_SYS_IFNULL,   ODBCXX_STRING_CONST("IFNULL") },
    { SQL_FN_SYS_USERNAME, ODBCXX_STRING_CONST("USERNAME") },
    { 0, NULL }
  };

  SQLUINTEGER r=this->_getNumeric32
    (SQL_SYSTEM_FUNCTIONS);

  ODBCXX_STRING ret=ODBCXX_STRING_CONST("");
  for(int i=0; fmap[i].funcId>0; i++) {
    if(r&(fmap[i].funcId)) {
      if(ret.length()>0) {
	ret+=ODBCXX_STRING_CONST(",");
      }
      ret+=fmap[i].funcName;
    }
  }
  return ret;
}


ODBCXX_STRING DatabaseMetaData::getTimeDateFunctions()
{
  static struct {
    int funcId;
    const ODBCXX_CHAR_TYPE* funcName;
  } fmap[] = {
#if ODBCVER>=0x0300
    { SQL_FN_TD_CURRENT_DATE,      ODBCXX_STRING_CONST("CURRENT_DATE") },
    { SQL_FN_TD_CURRENT_TIME,      ODBCXX_STRING_CONST("CURRENT_TIME") },
    { SQL_FN_TD_CURRENT_TIMESTAMP, ODBCXX_STRING_CONST("CURRENT_TIMESTAMP") },
    { SQL_FN_TD_EXTRACT,           ODBCXX_STRING_CONST("EXTRACT") },
#endif
    { SQL_FN_TD_CURDATE,           ODBCXX_STRING_CONST("CURDATE") },
    { SQL_FN_TD_CURTIME,           ODBCXX_STRING_CONST("CURTIME") },
    { SQL_FN_TD_DAYNAME,           ODBCXX_STRING_CONST("DAYNAME") },
    { SQL_FN_TD_DAYOFMONTH,        ODBCXX_STRING_CONST("DAYOFMONTH") },
    { SQL_FN_TD_DAYOFWEEK,         ODBCXX_STRING_CONST("DAYOFWEEK") },
    { SQL_FN_TD_DAYOFYEAR,         ODBCXX_STRING_CONST("DAYOFYEAR") },
    { SQL_FN_TD_HOUR,              ODBCXX_STRING_CONST("HOUR") },
    { SQL_FN_TD_MINUTE,            ODBCXX_STRING_CONST("MINUTE") },
    { SQL_FN_TD_MONTH,             ODBCXX_STRING_CONST("MONTH") },
    { SQL_FN_TD_MONTHNAME,         ODBCXX_STRING_CONST("MONTHNAME") },
    { SQL_FN_TD_NOW,               ODBCXX_STRING_CONST("NOW") },
    { SQL_FN_TD_QUARTER,           ODBCXX_STRING_CONST("QUARTER") },
    { SQL_FN_TD_SECOND,            ODBCXX_STRING_CONST("SECOND") },
    { SQL_FN_TD_TIMESTAMPADD,      ODBCXX_STRING_CONST("TIMESTAMPADD") },
    { SQL_FN_TD_TIMESTAMPDIFF,     ODBCXX_STRING_CONST("TIMESTAMPDIFF") },
    { SQL_FN_TD_WEEK,              ODBCXX_STRING_CONST("WEEK") },
    { SQL_FN_TD_YEAR,              ODBCXX_STRING_CONST("YEAR") },
    { 0, NULL }
  };

  SQLUINTEGER r=this->_getNumeric32
    (SQL_TIMEDATE_FUNCTIONS);

  ODBCXX_STRING ret=ODBCXX_STRING_CONST("");
  for(int i=0; fmap[i].funcId>0; i++) {
    if(r&(fmap[i].funcId)) {
      if(ret.length()>0) {
	ret+=ODBCXX_STRING_CONST(",");
      }
      ret+=fmap[i].funcName;
    }
  }
  return ret;
}


bool DatabaseMetaData::supportsCatalogsInDataManipulation()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_CATALOG_USAGE,
		   SQL_QUALIFIER_USAGE))
	  &ODBC3_C(SQL_CU_DML_STATEMENTS,
		   SQL_QU_DML_STATEMENTS))!=0;
}


bool DatabaseMetaData::supportsCatalogsInProcedureCalls()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_CATALOG_USAGE,
		   SQL_QUALIFIER_USAGE))
	  &ODBC3_C(SQL_CU_PROCEDURE_INVOCATION,
		   SQL_QU_PROCEDURE_INVOCATION))!=0;
}

bool DatabaseMetaData::supportsCatalogsInTableDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_CATALOG_USAGE,
		   SQL_QUALIFIER_USAGE))
	  &ODBC3_C(SQL_CU_TABLE_DEFINITION,
		   SQL_QU_TABLE_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsCatalogsInIndexDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_CATALOG_USAGE,
		   SQL_QUALIFIER_USAGE))
	  &ODBC3_C(SQL_CU_INDEX_DEFINITION,
		   SQL_QU_INDEX_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsCatalogsInPrivilegeDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_CATALOG_USAGE,
		   SQL_QUALIFIER_USAGE))
	  &ODBC3_C(SQL_CU_PRIVILEGE_DEFINITION,
		   SQL_QU_PRIVILEGE_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsSchemasInDataManipulation()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_SCHEMA_USAGE,
		   SQL_OWNER_USAGE))
	  &ODBC3_C(SQL_SU_DML_STATEMENTS,
		   SQL_OU_DML_STATEMENTS))!=0;
}


bool DatabaseMetaData::supportsSchemasInProcedureCalls()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_SCHEMA_USAGE,
		   SQL_OWNER_USAGE))
	  &ODBC3_C(SQL_SU_PROCEDURE_INVOCATION,
		   SQL_OU_PROCEDURE_INVOCATION))!=0;
}

bool DatabaseMetaData::supportsSchemasInTableDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_SCHEMA_USAGE,
		   SQL_OWNER_USAGE))
	  &ODBC3_C(SQL_SU_TABLE_DEFINITION,
		   SQL_OU_TABLE_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsSchemasInIndexDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_SCHEMA_USAGE,
		   SQL_OWNER_USAGE))
	  &ODBC3_C(SQL_SU_INDEX_DEFINITION,
		   SQL_OU_INDEX_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsSchemasInPrivilegeDefinitions()
{
  return (this->_getNumeric32
	  (ODBC3_C(SQL_SCHEMA_USAGE,
		   SQL_OWNER_USAGE))
	  &ODBC3_C(SQL_SU_PRIVILEGE_DEFINITION,
		   SQL_OU_PRIVILEGE_DEFINITION))!=0;
}


bool DatabaseMetaData::supportsGroupBy()
{
  return this->_getNumeric16
    (SQL_GROUP_BY)!=SQL_GB_NOT_SUPPORTED;
}

bool DatabaseMetaData::supportsGroupByUnrelated()
{
  SQLUSMALLINT r=this->_getNumeric16
    (SQL_GROUP_BY);
  return r==SQL_GB_NO_RELATION;
}

bool DatabaseMetaData::supportsGroupByBeyondSelect()
{
  SQLUSMALLINT r=this->_getNumeric16
    (SQL_GROUP_BY);
  return r==SQL_GB_GROUP_BY_CONTAINS_SELECT;
}


bool DatabaseMetaData::supportsUnion()
{
  return (this->_getNumeric32
	  (SQL_UNION)&SQL_U_UNION)!=0;
}

bool DatabaseMetaData::supportsUnionAll()
{
  return (this->_getNumeric32
	  (SQL_UNION)&(SQL_U_UNION | SQL_U_UNION_ALL))!=0;
}


bool DatabaseMetaData::supportsOuterJoins()
{
  return (this->_getNumeric32
	  (SQL_OJ_CAPABILITIES)&
	  (SQL_OJ_LEFT | SQL_OJ_RIGHT | SQL_OJ_FULL | SQL_OJ_NESTED))!=0;
}

bool DatabaseMetaData::supportsFullOuterJoins()
{
  return (this->_getNumeric32
	  (SQL_OJ_CAPABILITIES)&(SQL_OJ_FULL | SQL_OJ_NESTED))!=0;
}

bool DatabaseMetaData::supportsLimitedOuterJoins()
{
  return this->supportsFullOuterJoins() || this->supportsOuterJoins();
}


bool DatabaseMetaData::usesLocalFilePerTable()
{
  return this->_getNumeric16
    (SQL_FILE_USAGE)==SQL_FILE_TABLE;
}

bool DatabaseMetaData::usesLocalFiles()
{
  return this->_getNumeric16
    (SQL_FILE_USAGE)!=SQL_FILE_NOT_SUPPORTED;
}

bool DatabaseMetaData::nullsAreSortedHigh()
{
  return this->_getNumeric16
    (SQL_NULL_COLLATION)==SQL_NC_HIGH;
}

bool DatabaseMetaData::nullsAreSortedLow()
{
  return this->_getNumeric16
    (SQL_NULL_COLLATION)==SQL_NC_LOW;
}

bool DatabaseMetaData::nullsAreSortedAtStart()
{
  return this->_getNumeric16
    (SQL_NULL_COLLATION)==SQL_NC_START;
}

bool DatabaseMetaData::nullsAreSortedAtEnd()
{
  return this->_getNumeric16
    (SQL_NULL_COLLATION)==SQL_NC_END;
}

bool DatabaseMetaData::allProceduresAreCallable()
{
  return this->_getStringInfo(SQL_ACCESSIBLE_PROCEDURES)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::allTablesAreSelectable()
{
  return this->_getStringInfo(SQL_ACCESSIBLE_TABLES)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::isReadOnly()
{
  return this->_getStringInfo(SQL_DATA_SOURCE_READ_ONLY)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::supportsTableCorrelationNames()
{
  return this->_getNumeric16
    (SQL_CORRELATION_NAME)!=SQL_CN_NONE;
}

bool DatabaseMetaData::supportsCorrelatedSubqueries()
{
  return (this->_getNumeric32
	  (SQL_SUBQUERIES)&SQL_SQ_CORRELATED_SUBQUERIES)!=0;
}

bool DatabaseMetaData::supportsSubqueriesInComparisons()
{
  return (this->_getNumeric32
	  (SQL_SUBQUERIES)&SQL_SQ_COMPARISON)!=0;
}

bool DatabaseMetaData::supportsSubqueriesInExists()
{
  return (this->_getNumeric32
	  (SQL_SUBQUERIES)&SQL_SQ_EXISTS)!=0;
}

bool DatabaseMetaData::supportsSubqueriesInIns()
{
  return (this->_getNumeric32
	  (SQL_SUBQUERIES)&SQL_SQ_IN)!=0;
}

bool DatabaseMetaData::supportsSubqueriesInQuantifieds()
{
  return (this->_getNumeric32
	  (SQL_SUBQUERIES)&SQL_SQ_QUANTIFIED)!=0;
}


bool DatabaseMetaData::supportsExpressionsInOrderBy()
{
  return this->_getStringInfo(SQL_EXPRESSIONS_IN_ORDERBY)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::supportsLikeEscapeClause()
{
  return this->_getStringInfo(SQL_LIKE_ESCAPE_CLAUSE)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::supportsMultipleResultSets()
{
  return this->_getStringInfo(SQL_MULT_RESULT_SETS)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::supportsNonNullableColumns()
{
  return this->_getNumeric16
    (SQL_NON_NULLABLE_COLUMNS)==SQL_NNC_NON_NULL;
}

bool DatabaseMetaData::supportsMinimumSQLGrammar()
{
  return true;
}

bool DatabaseMetaData::supportsCoreSQLGrammar()
{
  return this->_getNumeric16
    (SQL_ODBC_SQL_CONFORMANCE)!=SQL_OSC_MINIMUM;
}

bool DatabaseMetaData::supportsExtendedSQLGrammar()
{
  return this->_getNumeric16
    (SQL_ODBC_SQL_CONFORMANCE)==SQL_OSC_EXTENDED;
}


bool DatabaseMetaData::supportsANSI92EntryLevelSQL()
{
  return ODBC3_DC((this->_getNumeric32
		   (SQL_SQL_CONFORMANCE)&SQL_SC_SQL92_ENTRY)!=0,
		  false);
}

bool DatabaseMetaData::supportsANSI92FullSQL()
{
  return ODBC3_DC((this->_getNumeric32
		   (SQL_SQL_CONFORMANCE)&SQL_SC_SQL92_FULL)!=0,
		  false);
}

bool DatabaseMetaData::supportsANSI92IntermediateSQL()
{
  return ODBC3_DC((this->_getNumeric32
		   (SQL_SQL_CONFORMANCE)&SQL_SC_SQL92_INTERMEDIATE)!=0,
		  false);
}


int
DatabaseMetaData::getMaxBinaryLiteralLength()
{
  return this->_getNumeric32
    (SQL_MAX_BINARY_LITERAL_LEN);
}

int
DatabaseMetaData::getMaxCharLiteralLength()
{
  return this->_getNumeric32
    (SQL_MAX_CHAR_LITERAL_LEN);
}

int
DatabaseMetaData::getMaxColumnNameLength()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMN_NAME_LEN);
}


int
DatabaseMetaData::getMaxColumnsInGroupBy()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMNS_IN_GROUP_BY);
}

int
DatabaseMetaData::getMaxColumnsInIndex()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMNS_IN_INDEX);
}

int
DatabaseMetaData::getMaxColumnsInOrderBy()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMNS_IN_ORDER_BY);
}


int
DatabaseMetaData::getMaxColumnsInSelect()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMNS_IN_SELECT);
}

int
DatabaseMetaData::getMaxColumnsInTable()
{
  return this->_getNumeric16
    (SQL_MAX_COLUMNS_IN_TABLE);
}

int
DatabaseMetaData::getMaxCursorNameLength()
{
  return this->_getNumeric16
    (SQL_MAX_CURSOR_NAME_LEN);
}

int
DatabaseMetaData::getMaxIndexLength()
{
  return this->_getNumeric32
    (SQL_MAX_INDEX_SIZE);
}

int
DatabaseMetaData::getMaxSchemaNameLength()
{
  return this->_getNumeric16
    (ODBC3_C(SQL_MAX_SCHEMA_NAME_LEN,SQL_MAX_OWNER_NAME_LEN));
}

int
DatabaseMetaData::getMaxProcedureNameLength()
{
  return this->_getNumeric16
    (SQL_MAX_PROCEDURE_NAME_LEN);
}

int
DatabaseMetaData::getMaxCatalogNameLength()
{
  return this->_getNumeric16
    (ODBC3_C(SQL_MAX_CATALOG_NAME_LEN,SQL_MAX_QUALIFIER_NAME_LEN));
}

int
DatabaseMetaData::getMaxRowSize()
{
  return this->_getNumeric32
    (SQL_MAX_ROW_SIZE);
}

bool DatabaseMetaData::doesMaxRowSizeIncludeBlobs()
{
  return this->_getStringInfo
    (SQL_MAX_ROW_SIZE_INCLUDES_LONG)==ODBCXX_STRING_CONST("Y");
}

int
DatabaseMetaData::getMaxStatementLength()
{
  return this->_getNumeric32
    (SQL_MAX_STATEMENT_LEN);
}

int
DatabaseMetaData::getMaxTableNameLength()
{
  return this->_getNumeric16
    (SQL_MAX_TABLE_NAME_LEN);
}

int
DatabaseMetaData::getMaxTablesInSelect()
{
  return this->_getNumeric16
    (SQL_MAX_TABLES_IN_SELECT);
}

int
DatabaseMetaData::getMaxUserNameLength()
{
  return this->_getNumeric16
    (SQL_MAX_USER_NAME_LEN);
}


int
DatabaseMetaData::getMaxConnections()
{
  return (int)this->_getNumeric16
    (ODBC3_C(SQL_MAX_DRIVER_CONNECTIONS,SQL_ACTIVE_CONNECTIONS));
}


int
DatabaseMetaData::getMaxStatements()
{
  return (int)this->_getNumeric16
    (ODBC3_C(SQL_MAX_CONCURRENT_ACTIVITIES,SQL_ACTIVE_STATEMENTS));
}



bool DatabaseMetaData::supportsMultipleTransactions()
{
  return this->_getStringInfo(SQL_MULTIPLE_ACTIVE_TXN)==ODBCXX_STRING_CONST("Y");
}


bool DatabaseMetaData::supportsOrderByUnrelated()
{
  return this->_getStringInfo(SQL_ORDER_BY_COLUMNS_IN_SELECT)!=ODBCXX_STRING_CONST("Y");
}



bool DatabaseMetaData::supportsDifferentTableCorrelationNames()
{
  return this->_getNumeric16
    (SQL_CORRELATION_NAME)==SQL_CN_DIFFERENT;
}

bool DatabaseMetaData::supportsConvert()
{
  return (this->_getNumeric32
	  (SQL_CONVERT_FUNCTIONS)&SQL_FN_CVT_CONVERT)!=0;
}

bool DatabaseMetaData::supportsConvert(int fromType,
				       int toType)
{
  static const int numTypes=19;

  static struct {
    int id;
    int value;
  } convertMap[numTypes] = {
    { Types::BIGINT, SQL_CONVERT_BIGINT },
    { Types::BINARY, SQL_CONVERT_BINARY },
    { Types::BIT, SQL_CONVERT_BIT },
    { Types::CHAR, SQL_CONVERT_CHAR },
    { Types::DATE, SQL_CONVERT_DATE },
    { Types::DECIMAL, SQL_CONVERT_DECIMAL },
    { Types::DOUBLE, SQL_CONVERT_DOUBLE },
    { Types::FLOAT, SQL_CONVERT_FLOAT },
    { Types::INTEGER, SQL_CONVERT_INTEGER },
    { Types::LONGVARBINARY, SQL_CONVERT_LONGVARBINARY },
    { Types::LONGVARCHAR, SQL_CONVERT_LONGVARCHAR },
    { Types::NUMERIC, SQL_CONVERT_NUMERIC },
    { Types::REAL, SQL_CONVERT_REAL },
    { Types::SMALLINT, SQL_CONVERT_SMALLINT },
    { Types::TIME, SQL_CONVERT_TIME },
    { Types::TIMESTAMP, SQL_CONVERT_TIMESTAMP },
    { Types::TINYINT, SQL_CONVERT_TINYINT },
    { Types::VARBINARY, SQL_CONVERT_VARBINARY },
    { Types::VARCHAR, SQL_CONVERT_VARCHAR }
  };

  static struct {
    int id;
    int value;
  } cvtMap[numTypes] = {
    { Types::BIGINT, SQL_CVT_BIGINT },
    { Types::BINARY, SQL_CVT_BINARY },
    { Types::BIT, SQL_CVT_BIT },
    { Types::CHAR, SQL_CVT_CHAR },
    { Types::DATE, SQL_CVT_DATE },
    { Types::DECIMAL, SQL_CVT_DECIMAL },
    { Types::DOUBLE, SQL_CVT_DOUBLE },
    { Types::FLOAT, SQL_CVT_FLOAT },
    { Types::INTEGER, SQL_CVT_INTEGER },
    { Types::LONGVARBINARY, SQL_CVT_LONGVARBINARY },
    { Types::LONGVARCHAR, SQL_CVT_LONGVARCHAR },
    { Types::NUMERIC, SQL_CVT_NUMERIC },
    { Types::REAL, SQL_CVT_REAL },
    { Types::SMALLINT, SQL_CVT_SMALLINT },
    { Types::TIME, SQL_CVT_TIME },
    { Types::TIMESTAMP, SQL_CVT_TIMESTAMP },
    { Types::TINYINT, SQL_CVT_TINYINT },
    { Types::VARBINARY, SQL_CVT_VARBINARY },
    { Types::VARCHAR, SQL_CVT_VARCHAR }
  };

  for(int i=0; i<numTypes; i++) {
    if(convertMap[i].value==fromType) {
      for(int j=0; j<numTypes; j++) {
	if(cvtMap[j].value==toType) {
	  return (this->_getNumeric32
		  (convertMap[i].value)&cvtMap[i].value)!=0;
	}
      }
      throw SQLException
	(ODBCXX_STRING_CONST("[libodbc++]: supportsConvert(): Unknown toType ")+
	 intToString(toType), SQLException::scDEFSQLSTATE);
    }
  }
  throw SQLException
    (ODBCXX_STRING_CONST("[libodbc++]: supportsConvert(): Unknown fromType ")+
     intToString(fromType), SQLException::scDEFSQLSTATE);

  ODBCXX_DUMMY_RETURN(false);
}


bool DatabaseMetaData::storesLowerCaseIdentifiers()
{
  return this->_getNumeric16
    (SQL_IDENTIFIER_CASE)==SQL_IC_LOWER;
}


bool DatabaseMetaData::storesLowerCaseQuotedIdentifiers()
{
  return this->_getNumeric16
    (SQL_QUOTED_IDENTIFIER_CASE)==SQL_IC_LOWER;
}


bool DatabaseMetaData::storesMixedCaseIdentifiers()
{
  return this->_getNumeric16
    (SQL_IDENTIFIER_CASE)==SQL_IC_MIXED;
}


bool DatabaseMetaData::storesMixedCaseQuotedIdentifiers()
{
  return this->_getNumeric16
    (SQL_QUOTED_IDENTIFIER_CASE)==SQL_IC_MIXED;
}


bool DatabaseMetaData::storesUpperCaseIdentifiers()
{
  return this->_getNumeric16
    (SQL_IDENTIFIER_CASE)==SQL_IC_UPPER;
}


bool DatabaseMetaData::storesUpperCaseQuotedIdentifiers()
{
  return this->_getNumeric16
    (SQL_QUOTED_IDENTIFIER_CASE)==SQL_IC_UPPER;
}


bool DatabaseMetaData::supportsMixedCaseIdentifiers()
{
  return this->_getNumeric16
    (SQL_IDENTIFIER_CASE)==SQL_IC_SENSITIVE;
}

bool DatabaseMetaData::supportsMixedCaseQuotedIdentifiers()
{
  return this->_getNumeric16
    (SQL_QUOTED_IDENTIFIER_CASE)==SQL_IC_SENSITIVE;
}


bool DatabaseMetaData::supportsStoredProcedures()
{
  return this->_getStringInfo(SQL_PROCEDURES)==ODBCXX_STRING_CONST("Y");
}

bool DatabaseMetaData::_ownXXXAreVisible(int type, int what)
{
  int odbcType=getODBCCursorTypeFor(type,this->_getDriverInfo());

#if ODBCVER >= 0x0300
  if(this->_getDriverInfo()->getMajorVersion()>2) {
    SQLUINTEGER r=this->_getNumeric32
      (getCursorAttributes2For(odbcType));

    switch(what) {
    case UPDATES:
      return (r&SQL_CA2_SENSITIVITY_UPDATES)!=0;
    case INSERTS:
      return (r&SQL_CA2_SENSITIVITY_ADDITIONS)!=0;
    case DELETES:
      return (r&SQL_CA2_SENSITIVITY_DELETIONS)!=0;
    }
    //notreached
    assert(false);
  }
#endif

  // for odbc 2, assume false for forward only, true for dynamic
  // and check for static and keyset driven

  switch(odbcType) {
  case SQL_CURSOR_FORWARD_ONLY:
    return false;

  case SQL_CURSOR_DYNAMIC:
    return true;

  case SQL_CURSOR_KEYSET_DRIVEN:
  case SQL_CURSOR_STATIC:
    {
      SQLUINTEGER r=this->_getNumeric32(SQL_STATIC_SENSITIVITY);
      switch(what) {
      case UPDATES:
	return (r&SQL_SS_UPDATES)!=0;
      case INSERTS:
	return (r&SQL_SS_ADDITIONS)!=0;
      case DELETES:
	return (r&SQL_SS_DELETIONS)!=0;
      }
    }
  }

  // notreached
  assert(false);
  ODBCXX_DUMMY_RETURN(false);
}

bool DatabaseMetaData::ownUpdatesAreVisible(int type)
{
  return this->_ownXXXAreVisible(type,UPDATES);
}

bool DatabaseMetaData::ownDeletesAreVisible(int type)
{
  return this->_ownXXXAreVisible(type,DELETES);
}

bool DatabaseMetaData::ownInsertsAreVisible(int type)
{
  return this->_ownXXXAreVisible(type,INSERTS);
}

// If I get this correct, the next 3 methods are only
// true when we're using a dynamic cursor

bool DatabaseMetaData::othersUpdatesAreVisible(int type)
{
  int ct=getODBCCursorTypeFor(type,this->_getDriverInfo());
  return ct==SQL_CURSOR_DYNAMIC;
}

bool DatabaseMetaData::othersInsertsAreVisible(int type)
{
  int ct=getODBCCursorTypeFor(type,this->_getDriverInfo());
  return ct==SQL_CURSOR_DYNAMIC;
}

bool DatabaseMetaData::othersDeletesAreVisible(int type)
{
  int ct=getODBCCursorTypeFor(type,this->_getDriverInfo());
  return ct==SQL_CURSOR_DYNAMIC;
}

bool DatabaseMetaData::deletesAreDetected(int type)
{
  return (type!=ResultSet::TYPE_FORWARD_ONLY &&
	  !this->ownDeletesAreVisible(type));
}

bool DatabaseMetaData::insertsAreDetected(int type)
{
  return (type!=ResultSet::TYPE_FORWARD_ONLY &&
	  this->ownInsertsAreVisible(type));
}

bool DatabaseMetaData::updatesAreDetected(int type)
{
  return (type!=ResultSet::TYPE_FORWARD_ONLY);
}


//catalog stuff - actually implemented in Statement
ResultSet* DatabaseMetaData::getTypeInfo()
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getTypeInfo();
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}

ResultSet* DatabaseMetaData::getTables(const ODBCXX_STRING& catalog,
				       const ODBCXX_STRING& schemaPattern,
				       const ODBCXX_STRING& tableNamePattern,
				       const vector<ODBCXX_STRING>& types)
{
  ODBCXX_STRING typesStr;
  for(unsigned int i=0; i<types.size(); i++) {
    if(i>0) {
      typesStr+=ODBCXX_STRING_CONST(",");
    }
    typesStr+=types[i];
  }

  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getTables(catalog,
			    schemaPattern,
			    tableNamePattern,
			    typesStr);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}

ResultSet* DatabaseMetaData::getColumns(const ODBCXX_STRING& catalog,
					const ODBCXX_STRING& schemaPattern,
					const ODBCXX_STRING& tableNamePattern,
					const ODBCXX_STRING& columnNamePattern)
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getColumns(catalog,
			     schemaPattern,
			     tableNamePattern,
			     columnNamePattern);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}

ResultSet* DatabaseMetaData::getTablePrivileges(const ODBCXX_STRING& catalog,
						const ODBCXX_STRING& schemaPattern,
						const ODBCXX_STRING& tableNamePattern)
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getTablePrivileges(catalog,
				     schemaPattern,
				     tableNamePattern);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}

ResultSet* DatabaseMetaData::getColumnPrivileges(const ODBCXX_STRING& catalog,
						 const ODBCXX_STRING& schemaPattern,
						 const ODBCXX_STRING& tableNamePattern,
						 const ODBCXX_STRING& columnNamePattern)
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getColumnPrivileges(catalog,
				      schemaPattern,
				      tableNamePattern,
				      columnNamePattern);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getPrimaryKeys(const ODBCXX_STRING& catalog,
					    const ODBCXX_STRING& schema,
					    const ODBCXX_STRING& table)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getPrimaryKeys(catalog,
				 schema,
				 table);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getCrossReference(const ODBCXX_STRING& primaryCatalog,
					       const ODBCXX_STRING& primarySchema,
					       const ODBCXX_STRING& primaryTable,
					       const ODBCXX_STRING& foreignCatalog,
					       const ODBCXX_STRING& foreignSchema,
					       const ODBCXX_STRING& foreignTable)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getCrossReference(primaryCatalog,
				    primarySchema,
				    primaryTable,
				    foreignCatalog,
				    foreignSchema,
				    foreignTable);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}



ResultSet* DatabaseMetaData::getTableTypes()
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getTables(ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST("%"));
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getSchemas()
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getTables(ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST("%"),
                            ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST(""));
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getCatalogs()
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getTables(ODBCXX_STRING_CONST("%"),
                            ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST(""),
                            ODBCXX_STRING_CONST(""));
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getIndexInfo(const ODBCXX_STRING& catalog,
					  const ODBCXX_STRING& schemaPattern,
					  const ODBCXX_STRING& tableNamePattern,
					  bool unique, bool accurate)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getIndexInfo(catalog,schemaPattern,tableNamePattern,
			       unique,accurate);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getProcedures(const ODBCXX_STRING& catalog,
					   const ODBCXX_STRING& schemaPattern,
					   const ODBCXX_STRING& procedureNamePattern)
{
  Statement* stmt=connection_->createStatement();
  try {
    return stmt->_getProcedures(catalog,schemaPattern,procedureNamePattern);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getProcedureColumns(const ODBCXX_STRING& catalog,
						 const ODBCXX_STRING& schemaPattern,
						 const ODBCXX_STRING& procedureNamePattern,
						 const ODBCXX_STRING& columnNamePattern)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getProcedureColumns(catalog,
				      schemaPattern,
				      procedureNamePattern,
				      columnNamePattern);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}


ResultSet* DatabaseMetaData::getBestRowIdentifier(const ODBCXX_STRING& catalog,
						  const ODBCXX_STRING& schema,
						  const ODBCXX_STRING& table,
						  int scope,
						  bool nullable)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getSpecialColumns(catalog,schema,table,
				    SQL_BEST_ROWID,
				    scope,
				    nullable?SQL_NULLABLE:SQL_NO_NULLS);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}
						

ResultSet* DatabaseMetaData::getVersionColumns(const ODBCXX_STRING& catalog,
					       const ODBCXX_STRING& schema,
					       const ODBCXX_STRING& table)
{
  Statement* stmt=connection_->createStatement();

  try {
    return stmt->_getSpecialColumns(catalog,schema,table,
				    SQL_ROWVER,SQL_SCOPE_CURROW,
				    SQL_NULLABLE);
  } catch(...) {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stmt;
    throw;
  }
}
