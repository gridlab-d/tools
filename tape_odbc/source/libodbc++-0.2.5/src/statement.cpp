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

#include <odbc++/statement.h>
#include <odbc++/resultset.h>
#include <odbc++/connection.h>
#include "driverinfo.h"

#include "dtconv.h"

using namespace odbc;
using namespace std;

Statement::Statement(Connection* con, SQLHSTMT hstmt,
                     int resultSetType, int resultSetConcurrency)
  :connection_(con),
  hstmt_(hstmt),
  lastExecute_(SQL_SUCCESS),
  currentResultSet_(NULL),
  fetchSize_(SQL_ROWSET_SIZE_DEFAULT),
  resultSetType_(resultSetType),
  resultSetConcurrency_(resultSetConcurrency),
  state_(STATE_CLOSED)
{
  try {

    this->_applyResultSetType();

  } catch(...) {
    // avoid a statement handle leak (the destructor will not be called)
#if ODBCVER < 0x0300
    SQLFreeStmt(hstmt_,SQL_DROP);
#else
    SQLFreeHandle(SQL_HANDLE_STMT,hstmt_);
#endif
    throw;
  }
}

Statement::~Statement()
{
  if(currentResultSet_!=NULL) {
    currentResultSet_->ownStatement_=false;
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) currentResultSet_;
    currentResultSet_=NULL;
  }

#if ODBCVER < 0x0300
    SQLFreeStmt(hstmt_,SQL_DROP);
#else
    SQLFreeHandle(SQL_HANDLE_STMT,hstmt_);
#endif

  connection_->_unregisterStatement(this);
}


//private
void Statement::_registerResultSet(ResultSet* rs)
{
  assert(currentResultSet_==NULL);
  currentResultSet_=rs;
}

void Statement::_unregisterResultSet(ResultSet* rs)
{
  assert(currentResultSet_==rs);
  currentResultSet_=NULL;
}


//protected
SQLUINTEGER Statement::_getUIntegerOption(SQLINTEGER optnum)
{
  SQLUINTEGER res;
  SQLRETURN r;

#if ODBCVER < 0x0300

  r=SQLGetStmtOption(hstmt_,optnum,(SQLPOINTER)&res);

#else

  SQLINTEGER dummy;
  r=SQLGetStmtAttr(hstmt_,optnum,(SQLPOINTER)&res,SQL_IS_UINTEGER,&dummy);

#endif

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching numeric statement option"));

  return res;
}

//protected
void Statement::_setUIntegerOption(SQLINTEGER optnum, SQLUINTEGER value)
{
  SQLRETURN r;

#if ODBCVER < 0x0300

  r=SQLSetStmtOption(hstmt_,optnum,value);

#else


  r=SQLSetStmtAttr(hstmt_,optnum,(SQLPOINTER)value,SQL_IS_UINTEGER);

#endif

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error setting numeric statement option"));
}

//protected
ODBCXX_STRING Statement::_getStringOption(SQLINTEGER optnum)
{
  SQLRETURN r;
#if ODBCVER < 0x0300

  ODBCXX_CHAR_TYPE buf[SQL_MAX_OPTION_STRING_LENGTH+1];

  r=SQLGetStmtOption(hstmt_,optnum,(SQLPOINTER)buf);

  this->_checkStmtError(hstmt_,r,"Error fetching string statement option");

#else

  ODBCXX_CHAR_TYPE buf[256];
  SQLINTEGER dataSize;
  r=SQLGetStmtAttr(hstmt_,optnum,(SQLPOINTER)buf,255,&dataSize);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching string statement option"));

  if(dataSize>255) {
    // we have a longer attribute here
    ODBCXX_CHAR_TYPE* tmp=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[dataSize+1];
    odbc::Deleter<ODBCXX_CHAR_TYPE> _tmp(tmp,true);

    r=SQLGetStmtAttr(hstmt_,optnum,(SQLPOINTER)tmp,dataSize,&dataSize);

    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching string statement option"));
    return ODBCXX_STRING_C(tmp);
  }
#endif


  return ODBCXX_STRING_C(buf);
}

//protected
void Statement::_setStringOption(SQLINTEGER optnum,
                                 const ODBCXX_STRING& value)
{
  SQLRETURN r;

#if ODBCVER < 0x0300

  r=SQLSetStmtOption(hstmt_,optnum,
                     (SQLUINTEGER) ODBCXX_STRING_CSTR(value));

#else

  r=SQLSetStmtAttr(hstmt_,optnum,
                   (SQLPOINTER) ODBCXX_STRING_CSTR(value),
                   ODBCXX_STRING_LEN(value));

#endif

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error setting string statement option"));
}


#if ODBCVER >= 0x0300

SQLPOINTER Statement::_getPointerOption(SQLINTEGER optnum)
{
  SQLPOINTER ret;
  SQLINTEGER len;
  SQLRETURN r=SQLGetStmtAttr(hstmt_,optnum,(SQLPOINTER)&ret,
                             SQL_IS_POINTER,&len);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching pointer statement option"));

  return ret;
}


void Statement::_setPointerOption(SQLINTEGER optnum, SQLPOINTER value)
{
  SQLRETURN r=SQLSetStmtAttr(hstmt_,optnum,value,SQL_IS_POINTER);

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error setting pointer statement option"));
}


#endif



void Statement::_applyResultSetType()
{
  const DriverInfo* di=this->_getDriverInfo();

  int ct;

  switch(resultSetType_) {
  case ResultSet::TYPE_FORWARD_ONLY:
    ct=SQL_CURSOR_FORWARD_ONLY;
    break;

  case ResultSet::TYPE_SCROLL_INSENSITIVE:
    if(di->supportsStatic()) {
      ct=SQL_CURSOR_STATIC;
    } else {
      throw SQLException
        (ODBCXX_STRING_CONST("[libodbc++]: Datasource does not support ResultSet::TYPE_SCROLL_INSENSITIVE"), ODBCXX_STRING_CONST("S1C00"));
    }
    break;

  case ResultSet::TYPE_SCROLL_SENSITIVE:
    if(di->supportsScrollSensitive()) {
      ct=di->getScrollSensitive();
    } else {
      throw SQLException
        (ODBCXX_STRING_CONST("[libodbc++]: Datasource does not support ResultSet::TYPE_SCROLL_SENSITIVE"), ODBCXX_STRING_CONST("S1C00"));
    }
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid ResultSet type"), ODBCXX_STRING_CONST("S1009"));
  }

  if(ct!=SQL_CURSOR_FORWARD_ONLY) {
    this->_setUIntegerOption
      (ODBC3_C(SQL_ATTR_CURSOR_TYPE,SQL_CURSOR_TYPE),ct);
  }

  // concurrency:

  switch(resultSetConcurrency_) {
  case ResultSet::CONCUR_READ_ONLY:
    // we only apply this for non-default cursors
    if(ct!=SQL_CURSOR_FORWARD_ONLY) {
      if(di->supportsReadOnly(ct)) {
        this->_setUIntegerOption
          (ODBC3_C(SQL_ATTR_CONCURRENCY,SQL_CONCURRENCY),SQL_CONCUR_READ_ONLY);
      } else {
        throw SQLException
          (ODBCXX_STRING_CONST("[libodbc++]: ResultSet::CONCUR_READ_ONLY not supported for given type"), SQLException::scDEFSQLSTATE);
      }
    }
    break;

  case ResultSet::CONCUR_UPDATABLE:
    if(di->supportsUpdatable(ct)) {

      this->_setUIntegerOption
        (ODBC3_C(SQL_ATTR_CONCURRENCY,SQL_CONCURRENCY),
         di->getUpdatable(ct));

    } else {
      throw SQLException
        (ODBCXX_STRING_CONST("[libodbc++]: ResultSet::CONCUR_UPDATABLE not supported for given type"), SQLException::scDEFSQLSTATE);
    }
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Invalid concurrency level"), SQLException::scDEFSQLSTATE);
  }
}


//protected
bool Statement::_checkForResults()
{
  SQLSMALLINT nc;
  SQLRETURN r=SQLNumResultCols(hstmt_,&nc);
  return r==SQL_SUCCESS && nc>0;
}

//protected
ResultSet* Statement::_getResultSet(bool hideMe)
{
  ResultSet* rs=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ResultSet(this,hstmt_,hideMe);
  this->_registerResultSet(rs);
  return rs;
}


//protected
void Statement::_beforeExecute()
{
  this->clearWarnings();

  if(currentResultSet_!=NULL) {
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: Cannot re-execute; statement has an open resultset"), SQLException::scDEFSQLSTATE);
  }

  if(state_==STATE_OPEN) {
    SQLRETURN r=SQLFreeStmt(hstmt_,SQL_CLOSE);
    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error closing statement"), SQLException::scDEFSQLSTATE);

    state_=STATE_CLOSED;
  }
}

//protected
void Statement::_afterExecute()
{
  state_=STATE_OPEN;
}

//private catalog stuff
//this statement should be hidden behind a ResultSet
//but since it can be obtained with ResultSet->getStatement()
//we still track the state (before/afterExecute).

inline ODBCXX_SQLCHAR* valueOrNull(const ODBCXX_STRING& str)
{
  return (ODBCXX_SQLCHAR*)(ODBCXX_STRING_LEN(str)>0?
                    ODBCXX_STRING_DATA(str):NULL);
}

ResultSet* Statement::_getTypeInfo()
{
  this->_beforeExecute();

  SQLRETURN r=SQLGetTypeInfo(hstmt_,SQL_ALL_TYPES);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching type information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}


ResultSet* Statement::_getColumns(const ODBCXX_STRING& catalog,
                                  const ODBCXX_STRING& schema,
                                  const ODBCXX_STRING& tableName,
                                  const ODBCXX_STRING& columnName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLColumns(hstmt_,
                         valueOrNull(catalog),
                         ODBCXX_STRING_LEN(catalog),
                         valueOrNull(schema),
                         ODBCXX_STRING_LEN(schema),
                         valueOrNull(tableName),
                         ODBCXX_STRING_LEN(tableName),
                         valueOrNull(columnName),
                         ODBCXX_STRING_LEN(columnName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching column information"));

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}


ResultSet* Statement::_getTables(const ODBCXX_STRING& catalog,
                                 const ODBCXX_STRING& schema,
                                 const ODBCXX_STRING& tableName,
                                 const ODBCXX_STRING& types)
{
  this->_beforeExecute();
  SQLRETURN r=SQLTables(hstmt_,
                        valueOrNull(catalog),
                        ODBCXX_STRING_LEN(catalog),
                        valueOrNull(schema),
                        ODBCXX_STRING_LEN(schema),
                        valueOrNull(tableName),
                        ODBCXX_STRING_LEN(tableName),
                        (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(types),
                        ODBCXX_STRING_LEN(types));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching table information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);


  return rs;
}


ResultSet* Statement::_getTablePrivileges(const ODBCXX_STRING& catalog,
                                          const ODBCXX_STRING& schema,
                                          const ODBCXX_STRING& tableName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLTablePrivileges(hstmt_,
                                 valueOrNull(catalog),
                                 ODBCXX_STRING_LEN(catalog),
                                 valueOrNull(schema),
                                 ODBCXX_STRING_LEN(schema),
                                 (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(tableName),
                                 ODBCXX_STRING_LEN(tableName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching table privileges information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);


  return rs;
}

ResultSet* Statement::_getColumnPrivileges(const ODBCXX_STRING& catalog,
                                           const ODBCXX_STRING& schema,
                                           const ODBCXX_STRING& tableName,
                                           const ODBCXX_STRING& columnName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLColumnPrivileges(hstmt_,
                                  valueOrNull(catalog),
                                  ODBCXX_STRING_LEN(catalog),
                                  valueOrNull(schema),
                                  ODBCXX_STRING_LEN(schema),
                                  (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(tableName),
                                  ODBCXX_STRING_LEN(tableName),
                                  (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(columnName),
                                  ODBCXX_STRING_LEN(columnName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching column privileges information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);


  return rs;
}



ResultSet* Statement::_getPrimaryKeys(const ODBCXX_STRING& catalog,
                                      const ODBCXX_STRING& schema,
                                      const ODBCXX_STRING& tableName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLPrimaryKeys(hstmt_,
                             valueOrNull(catalog),
                             ODBCXX_STRING_LEN(catalog),
                             valueOrNull(schema),
                             ODBCXX_STRING_LEN(schema),
                             (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(tableName),
                             ODBCXX_STRING_LEN(tableName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching primary keys information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);


  return rs;
}


ResultSet* Statement::_getCrossReference(const ODBCXX_STRING& pc,
                                         const ODBCXX_STRING& ps,
                                         const ODBCXX_STRING& pt,
                                         const ODBCXX_STRING& fc,
                                         const ODBCXX_STRING& fs,
                                         const ODBCXX_STRING& ft)
{
  this->_beforeExecute();

  SQLRETURN r=SQLForeignKeys(hstmt_,
                             valueOrNull(pc),
                             ODBCXX_STRING_LEN(pc),
                             valueOrNull(ps),
                             ODBCXX_STRING_LEN(ps),
                             (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(pt),
                             ODBCXX_STRING_LEN(pt),
                             valueOrNull(fc),
                             ODBCXX_STRING_LEN(fc),
                             valueOrNull(fs),
                             ODBCXX_STRING_LEN(fs),
                             valueOrNull(ft),
//                             (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(ft),
                             ODBCXX_STRING_LEN(ft));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching foreign keys information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}



ResultSet* Statement::_getIndexInfo(const ODBCXX_STRING& catalog,
                                    const ODBCXX_STRING& schema,
                                    const ODBCXX_STRING& tableName,
                                    bool unique, bool approximate)
{
  this->_beforeExecute();
  SQLRETURN r=SQLStatistics(hstmt_,
                            valueOrNull(catalog),
                            ODBCXX_STRING_LEN(catalog),
                            valueOrNull(schema),
                            ODBCXX_STRING_LEN(schema),
                            (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(tableName),
                            ODBCXX_STRING_LEN(tableName),
                            unique?SQL_INDEX_UNIQUE:SQL_INDEX_ALL,
                            approximate?SQL_QUICK:SQL_ENSURE);

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching index information"));

  this->_afterExecute();

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}


ResultSet* Statement::_getProcedures(const ODBCXX_STRING& catalog,
                                     const ODBCXX_STRING& schema,
                                     const ODBCXX_STRING& procName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLProcedures(hstmt_,
                            valueOrNull(catalog),
                            ODBCXX_STRING_LEN(catalog),
                            valueOrNull(schema),
                            ODBCXX_STRING_LEN(schema),
                            (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(procName),
                            ODBCXX_STRING_LEN(procName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching procedures information"));

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}


ResultSet* Statement::_getProcedureColumns(const ODBCXX_STRING& catalog,
                                           const ODBCXX_STRING& schema,
                                           const ODBCXX_STRING& procName,
                                           const ODBCXX_STRING& colName)
{
  this->_beforeExecute();
  SQLRETURN r=SQLProcedureColumns(hstmt_,
                                  valueOrNull(catalog),
                                  ODBCXX_STRING_LEN(catalog),
                                  valueOrNull(schema),
                                  ODBCXX_STRING_LEN(schema),
                                  (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(procName),
                                  ODBCXX_STRING_LEN(procName),
                                  (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(colName),
                                  ODBCXX_STRING_LEN(colName));

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching procedures information"));

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}

ResultSet* Statement::_getSpecialColumns(const ODBCXX_STRING& catalog,
                                         const ODBCXX_STRING& schema,
                                         const ODBCXX_STRING& table,
                                         int what, int scope,
                                         int nullable)
{
  this->_beforeExecute();
  SQLRETURN r=SQLSpecialColumns(hstmt_,what,
                                valueOrNull(catalog),
                                ODBCXX_STRING_LEN(catalog),
                                valueOrNull(schema),
                                ODBCXX_STRING_LEN(schema),
                                (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(table),
                                ODBCXX_STRING_LEN(table),
                                scope,nullable);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching special columns"));

  ResultSet* rs=this->_getResultSet(true);

  return rs;
}



Connection* Statement::getConnection()
{
  return connection_;
}


int Statement::getQueryTimeout()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_QUERY_TIMEOUT,SQL_QUERY_TIMEOUT));
}


void Statement::setQueryTimeout(int seconds)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_QUERY_TIMEOUT,SQL_QUERY_TIMEOUT),seconds);
}


int Statement::getMaxRows()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_MAX_ROWS,SQL_MAX_ROWS));
}


void Statement::setMaxRows(int maxRows)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_MAX_ROWS,SQL_MAX_ROWS),maxRows);
}

int Statement::getMaxFieldSize()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_MAX_LENGTH,SQL_MAX_LENGTH));
}

void Statement::setMaxFieldSize(int maxFieldSize)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_MAX_LENGTH,SQL_MAX_LENGTH),maxFieldSize);
}


void Statement::cancel()
{
  SQLRETURN r=SQLCancel(hstmt_);
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error canceling statement"));
}


int Statement::getUpdateCount()
{
  // For ODBC3, if the last call to SQLExecute or SQLExecDirect
  // returned SQL_NO_DATA, a call to SQLRowCount can cause a
  // function sequence error. Therefore, if the last result is
  // SQL_NO_DATA, we simply return 0

  // Since an ODBC2 driver manager might be using

  if(lastExecute_!=ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {

    SQLLEN res;
    SQLRETURN r=SQLRowCount(hstmt_,&res);
    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error fetching update count"));
    return res;

  } else {
    return -1;
  }
}


void Statement::setCursorName(const ODBCXX_STRING& name)
{
  SQLRETURN r=SQLSetCursorName(hstmt_,
                               (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(name),
                               ODBCXX_STRING_LEN(name));
  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error setting cursor name"));
}


bool Statement::execute(const ODBCXX_STRING& sql)
{

  this->_beforeExecute();

  SQLRETURN r=SQLExecDirect(hstmt_,
                            (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(sql),
                            ODBCXX_STRING_LEN(sql));

  lastExecute_=r;

  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Error executing \"")+sql+ODBCXX_STRING_CONST("\"");

  this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CSTR(msg));

  this->_afterExecute();

  return this->_checkForResults();
}

ResultSet* Statement::executeQuery(const ODBCXX_STRING& sql)
{
  this->execute(sql);
  return this->_getResultSet();
}


int Statement::executeUpdate(const ODBCXX_STRING& sql)
{
  this->execute(sql);
  return this->getUpdateCount();
}

ResultSet* Statement::getResultSet()
{
  if(this->_checkForResults()) {
    return this->_getResultSet();
  }
  return NULL;
}


bool Statement::getMoreResults()
{
  if(this->_getDriverInfo()->supportsFunction(SQL_API_SQLMORERESULTS)) {
    SQLRETURN r=SQLMoreResults(hstmt_);
    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error checking for more results"), SQLException::scDEFSQLSTATE);
    // needed for getUpdateCount() to correctly
    // support the traversal of multiple results
    lastExecute_=r;

    return (r==SQL_SUCCESS ||
            r==SQL_SUCCESS_WITH_INFO);
  }
  return false;
}


void Statement::setFetchSize(int fs)
{
  if(fs>0) {
    fetchSize_=fs;
  } else if (fs==0) {
    fetchSize_=SQL_ROWSET_SIZE_DEFAULT;
  } else {
    throw SQLException(ODBCXX_STRING_CONST("Invalid fetch size"), SQLException::scDEFSQLSTATE);
  }
}


void Statement::setEscapeProcessing(bool on)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_NOSCAN,SQL_NOSCAN),on?SQL_NOSCAN_OFF:SQL_NOSCAN_ON);
}

bool Statement::getEscapeProcessing()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_NOSCAN,SQL_NOSCAN))==SQL_NOSCAN_OFF;
}

void Statement::close()
{
  if(state_==STATE_OPEN) {
    SQLRETURN r=SQLFreeStmt(hstmt_,SQL_CLOSE);
    this->_checkStmtError(hstmt_,r,ODBCXX_STRING_CONST("Error closing all results for statement"), SQLException::scDEFSQLSTATE);

    state_=STATE_CLOSED;
  }
}
