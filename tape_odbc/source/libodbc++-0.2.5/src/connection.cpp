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

#include <odbc++/connection.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/resultset.h>
#include <odbc++/callablestatement.h>

#include "driverinfo.h"

#include <set>

using namespace odbc;
using namespace std;

struct Connection::PD {
  typedef std::set<Statement*,std::less<Statement*> > StatementList;
  StatementList statements_;
  SQLHDBC hdbc_;
#ifdef ODBCXX_ENABLE_THREADS
  Mutex access_;
#endif
};

Connection::Connection(SQLHDBC h)
  :pd_(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) PD()),
   hdbc_(h),
   driverInfo_(NULL)
{
  metaData_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DatabaseMetaData(this);
}

Connection::~Connection()
{
  while(!pd_->statements_.empty()) {
    PD::StatementList::iterator
      i=pd_->statements_.begin();
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) *i;
  }

  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) metaData_;
  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) driverInfo_;

  //no error checking performed here
  SQLDisconnect(hdbc_);
#if ODBCVER < 0x0300
  SQLFreeConnect(hdbc_);
#else
  SQLFreeHandle(SQL_HANDLE_DBC,hdbc_);
#endif

  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) pd_;
}

//private
//We have a case where the calling statement won't be registered
//with this connection. When a constructor in a subclass
//of Statement fails, the Statement destructor will still
//be called.
void Connection::_unregisterStatement(Statement* stmt)
{
  ODBCXX_LOCKER(pd_->access_);
  PD::StatementList::iterator
    i=pd_->statements_.find(stmt);
  if(i!=pd_->statements_.end()) {
    pd_->statements_.erase(i);
  }
}

void Connection::_registerStatement(Statement* stmt)
{
  ODBCXX_LOCKER(pd_->access_);
  pd_->statements_.insert(pd_->statements_.end(),stmt);
}


//private
SQLUINTEGER Connection::_getUIntegerOption(SQLINTEGER optnum)
{
#if ODBCVER < 0x0300

  SQLUINTEGER res;
  SQLRETURN r=SQLGetConnectOption(hdbc_,optnum,(SQLPOINTER)&res);

  this->_checkConError(hdbc_,r,"Error fetching numeric connection option");

  return res;

#else
  // ODBC 3
  SQLUINTEGER res;
  SQLINTEGER dummy;
  SQLRETURN r=SQLGetConnectAttr(hdbc_,optnum,(SQLPOINTER)&res, sizeof(res),&dummy);
  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Error fetching numeric connection attribute"));
  return res;
#endif
}


//private
ODBCXX_STRING Connection::_getStringOption(SQLINTEGER optnum)
{
#if ODBCVER < 0x0300

  ODBCXX_CHAR_TYPE buf[SQL_MAX_OPTION_STRING_LENGTH+1]; //paranoia
  SQLRETURN r=SQLGetConnectOption(hdbc_,optnum,(ODBCXX_SQLCHAR*)buf);

  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Error fetching string connection option"));

  return ODBCXX_STRING_C(buf);

#else

  ODBCXX_CHAR_TYPE buf[256];
  SQLINTEGER optSize;
  SQLRETURN r=SQLGetConnectAttr(hdbc_,optnum,(SQLPOINTER)buf, 255, &optSize);

  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Error fetching string connection attribute"));

  if(optSize>255) {
    ODBCXX_CHAR_TYPE* tmp=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[optSize+1];
    odbc::Deleter<ODBCXX_CHAR_TYPE> _tmp(tmp,true);
    r=SQLGetConnectAttr(hdbc_,optnum,(SQLPOINTER)tmp,optSize,&optSize);
    this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Error fetching string connection attribute"));
    return ODBCXX_STRING_C(tmp);
  }
  return ODBCXX_STRING_C(buf);

#endif
}


//private
void Connection::_setIntegerOption(SQLINTEGER optnum, SQLINTEGER value)
{
  SQLRETURN r=
#if ODBCVER < 0x0300
    SQLSetConnectOption(hdbc_,optnum,value)
#else
    SQLSetConnectAttr(hdbc_,optnum,(SQLPOINTER)value,SQL_IS_INTEGER)
#endif
    ;
   this->_checkConError(hdbc_,r,"Error setting integer connection option");
}


//private
void Connection::_setUIntegerOption(SQLINTEGER optnum, SQLUINTEGER value)
{
  SQLRETURN r=
#if ODBCVER < 0x0300
    SQLSetConnectOption(hdbc_,optnum,value)
#else
    SQLSetConnectAttr(hdbc_,optnum,(SQLPOINTER)value,SQL_IS_UINTEGER)
#endif
    ;

  this->_checkConError(hdbc_,r,"Error setting unsigned integer connection option");
}


//private
void Connection::_setStringOption(SQLINTEGER optnum, const ODBCXX_STRING& value)
{
  SQLRETURN r=
#if ODBCVER < 0x0300
    SQLSetConnectOption(hdbc_,optnum,
                        (SQLUINTEGER)ODBCXX_STRING_CSTR(value))
#else
    SQLSetConnectAttr(hdbc_,optnum,
                      (SQLPOINTER)ODBCXX_STRING_CSTR(value),
                      ODBCXX_STRING_LEN(value));
#endif
    ;

  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Error setting string connection option"));
}


//private
void Connection::_connect(const ODBCXX_STRING& dsn,
                          const ODBCXX_STRING& user,
                          const ODBCXX_STRING& password)
{
  SQLRETURN r=SQLConnect(hdbc_,
                         (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(dsn),
                         ODBCXX_STRING_LEN(dsn),
                         (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(user),
                         ODBCXX_STRING_LEN(user),
                         (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(password),
                         ODBCXX_STRING_LEN(password));

  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Failed to connect to datasource"));

  driverInfo_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DriverInfo(this);
}

void Connection::_connect(const ODBCXX_STRING& connectString, SQLUSMALLINT drvcompl)
{
  ODBCXX_SQLCHAR dummy[256];
  SQLSMALLINT dummySize;
  SQLRETURN r=SQLDriverConnect(hdbc_,
                               0,
                               (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(connectString),
                               ODBCXX_STRING_LEN(connectString),
                               dummy,
                               255,
                               &dummySize,
                               drvcompl);

  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Failed to connect to datasource"));

  driverInfo_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DriverInfo(this);
}



bool Connection::getAutoCommit()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_AUTOCOMMIT,SQL_AUTOCOMMIT))==SQL_AUTOCOMMIT_ON;
}

void Connection::setAutoCommit(bool ac)
{
  this->_setIntegerOption
    (ODBC3_C(SQL_ATTR_AUTOCOMMIT,SQL_AUTOCOMMIT),
     ac?SQL_AUTOCOMMIT_ON:SQL_AUTOCOMMIT_OFF);
}


void Connection::commit()
{
  SQLRETURN r=
#if ODBCVER >= 0x0300
    SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_COMMIT)
#else
    SQLTransact(SQL_NULL_HENV, hdbc_, SQL_COMMIT)
#endif
    ;
  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Commit failed"));
}


void Connection::rollback()
{
  SQLRETURN r=
#if ODBCVER >= 0x0300
    SQLEndTran(SQL_HANDLE_DBC, hdbc_, SQL_ROLLBACK)
#else
    SQLTransact(SQL_NULL_HENV, hdbc_, SQL_ROLLBACK)
#endif
    ;
  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Rollback failed"));
}

void Connection::setCatalog(const ODBCXX_STRING& catalog)
{
  this->_setStringOption
    (ODBC3_C(SQL_ATTR_CURRENT_CATALOG,
             SQL_CURRENT_QUALIFIER), catalog);
}


ODBCXX_STRING Connection::getCatalog()
{
  return this->_getStringOption
    (ODBC3_C(SQL_ATTR_CURRENT_CATALOG,SQL_CURRENT_QUALIFIER));
}

Connection::TransactionIsolation
Connection::getTransactionIsolation()
{
  if(metaData_->supportsTransactions()) {
    SQLUINTEGER r=this->_getUIntegerOption
      (ODBC3_C(SQL_ATTR_TXN_ISOLATION,SQL_TXN_ISOLATION));
    switch(r) {
    case SQL_TXN_READ_UNCOMMITTED:
      return TRANSACTION_READ_UNCOMMITTED;

    case SQL_TXN_READ_COMMITTED:
      return TRANSACTION_READ_COMMITTED;

    case SQL_TXN_REPEATABLE_READ:
      return TRANSACTION_REPEATABLE_READ;

    case SQL_TXN_SERIALIZABLE:
#if ODBCVER < 0x0300 && defined SQL_TXN_VERSIONING
    case SQL_TXN_VERSIONING:
#endif
      return TRANSACTION_SERIALIZABLE;
    }
  }

  return TRANSACTION_NONE;
}

void Connection::setTransactionIsolation(TransactionIsolation i)
{
  if(metaData_->supportsTransactions()) {
    SQLUINTEGER l;
    switch(i) {
    case TRANSACTION_READ_UNCOMMITTED:
      l=SQL_TXN_READ_UNCOMMITTED;
      break;

    case TRANSACTION_READ_COMMITTED:
      l=SQL_TXN_READ_COMMITTED;
      break;

    case TRANSACTION_REPEATABLE_READ:
      l=SQL_TXN_REPEATABLE_READ;
      break;

    case TRANSACTION_SERIALIZABLE:
      l=SQL_TXN_SERIALIZABLE;
      break;

    default:
      throw SQLException
        (ODBCXX_STRING_CONST("[libodbc++]: Invalid transaction isolation"), ODBCXX_STRING_CONST("S1009"));
    }
    this->_setUIntegerOption
      (ODBC3_C(SQL_ATTR_TXN_ISOLATION,SQL_TXN_ISOLATION),l);
  } else {
    throw SQLException(ODBCXX_STRING_CONST("[libodbc++]: Data source does not support transactions"), ODBCXX_STRING_CONST("IM001"));
  }
}

void Connection::setReadOnly(bool readOnly)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_ACCESS_MODE, SQL_ACCESS_MODE),
     readOnly?SQL_MODE_READ_ONLY:SQL_MODE_READ_WRITE);
}

bool Connection::isReadOnly()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_ACCESS_MODE,SQL_ACCESS_MODE))==SQL_MODE_READ_ONLY;
}


bool Connection::getTrace()
{
  return this->_getUIntegerOption
    (ODBC3_C(SQL_ATTR_TRACE,SQL_OPT_TRACE))==SQL_OPT_TRACE_ON;
}

void Connection::setTrace(bool on)
{
  this->_setUIntegerOption
    (ODBC3_C(SQL_ATTR_TRACE,SQL_OPT_TRACE),
     on?SQL_OPT_TRACE_ON:SQL_OPT_TRACE_OFF);
}

ODBCXX_STRING Connection::getTraceFile()
{
  return this->_getStringOption
    (ODBC3_C(SQL_ATTR_TRACEFILE,SQL_OPT_TRACEFILE));
}

void Connection::setTraceFile(const ODBCXX_STRING& fn)
{
  this->_setStringOption
    (ODBC3_C(SQL_ATTR_TRACEFILE,SQL_OPT_TRACEFILE),
     fn);
}



ODBCXX_STRING Connection::nativeSQL(const ODBCXX_STRING& sql)
{
  ODBCXX_CHAR_TYPE buf[256];
  SQLINTEGER dataSize;
  SQLRETURN r;

  r=SQLNativeSql(hdbc_,
                 (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(sql),
                 ODBCXX_STRING_LEN(sql),
                 (ODBCXX_SQLCHAR*)buf,
                 255,
                 &dataSize);

  ODBCXX_STRING msg(ODBCXX_STRING_CONST("Error converting ")+sql+ODBCXX_STRING_CONST(" to native SQL"));
  this->_checkConError(hdbc_,r,
                       ODBCXX_STRING_CSTR(msg));

  if(dataSize>255) {
    ODBCXX_CHAR_TYPE* tmp=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[dataSize+1];
    odbc::Deleter<ODBCXX_CHAR_TYPE> _tmp(tmp,true);
    r=SQLNativeSql(hdbc_,
                   (ODBCXX_SQLCHAR*) ODBCXX_STRING_DATA(sql),
                   ODBCXX_STRING_LEN(sql),
                   (ODBCXX_SQLCHAR*)tmp,
                   dataSize+1,
                   &dataSize);
    this->_checkConError(hdbc_, r, ODBCXX_STRING_CSTR(msg));
    return ODBCXX_STRING_C(tmp);
  }

  return ODBCXX_STRING_C(buf);
}


DatabaseMetaData* Connection::getMetaData()
{
  return metaData_;
}


//private
SQLHSTMT Connection::_allocStmt()
{
  SQLHSTMT hstmt;
  SQLRETURN r=
#if ODBCVER < 0x0300
    SQLAllocStmt(hdbc_,&hstmt)
#else
    SQLAllocHandle(SQL_HANDLE_STMT,hdbc_,&hstmt)
#endif
    ;
  this->_checkConError(hdbc_,r,ODBCXX_STRING_CONST("Statement allocation failed"));
  return hstmt;
}


Statement* Connection::createStatement()
{
  //default values
  return this->createStatement(ResultSet::TYPE_FORWARD_ONLY,
                               ResultSet::CONCUR_READ_ONLY);
}

Statement* Connection::createStatement(int resultSetType,
                                       int resultSetConcurrency)
{
  SQLHSTMT hstmt=this->_allocStmt();

  Statement* stmt=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) Statement(this,hstmt,resultSetType,resultSetConcurrency);
  this->_registerStatement(stmt);
  return stmt;
}


PreparedStatement* Connection::prepareStatement(const ODBCXX_STRING& sql)
{
  //default values
  return this->prepareStatement(sql,
                                ResultSet::TYPE_FORWARD_ONLY,
                                ResultSet::CONCUR_READ_ONLY);
}

PreparedStatement* Connection::prepareStatement(const ODBCXX_STRING& sql,
                                                int resultSetType,
                                                int resultSetConcurrency)
{
  SQLHSTMT hstmt=this->_allocStmt();

  PreparedStatement* pstmt=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) PreparedStatement(this,hstmt,sql,
                                                 resultSetType,
                                                 resultSetConcurrency);
  this->_registerStatement(pstmt);
  return pstmt;
}


CallableStatement* Connection::prepareCall(const ODBCXX_STRING& sql)
{
  return this->prepareCall(sql,
                           ResultSet::TYPE_FORWARD_ONLY,
                           ResultSet::CONCUR_READ_ONLY);
}

CallableStatement* Connection::prepareCall(const ODBCXX_STRING& sql,
                                           int resultSetType,
                                           int resultSetConcurrency)
{
  SQLHSTMT hstmt=this->_allocStmt();

  CallableStatement* cstmt= ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) CallableStatement(this,hstmt,sql,
                                                  resultSetType,
                                                  resultSetConcurrency);
  this->_registerStatement(cstmt);
  return cstmt;
}
