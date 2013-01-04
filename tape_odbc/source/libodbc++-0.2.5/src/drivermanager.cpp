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

#include <odbc++/drivermanager.h>
#include <odbc++/errorhandler.h>
#include <odbc++/connection.h>

#include <memory>

using namespace odbc;
using namespace std;

SQLHENV DriverManager::henv_=SQL_NULL_HENV;
std::auto_ptr<ErrorHandler> DriverManager::eh_ = auto_ptr<ErrorHandler>();
SQLUSMALLINT	DriverManager::driverCompletion_ = SQL_DRIVER_COMPLETE;

//-1 means don't touch, 0 means wait forever, >0 means set it for every opened
//connection
int DriverManager::loginTimeout_=-1;


// Note: this will probably not work in all cases,
// static constructors are known to cause problems

#ifdef ODBCXX_ENABLE_THREADS

#define DMAccess DMAccessMutex()

odbc::Mutex & DMAccessMutex()
{
	// Here we wrap into auto ptr, because of failing on muliple shutdown calls.
	static std::auto_ptr<odbc::Mutex> mtx(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) odbc::Mutex());
	return *(mtx.get());
}

#endif /* ODBCXX_ENABLE_THREADS */


void DriverManager::shutdown()
{
  {
    ODBCXX_LOCKER(DMAccess);

    if (henv_ != SQL_NULL_HENV)
    {

      SQLRETURN r=ODBC3_C
        (SQLFreeHandle(SQL_HANDLE_ENV, henv_),
         SQLFreeEnv(henv_));

      switch(r)
      {
        case SQL_SUCCESS:
          break;

        case SQL_INVALID_HANDLE:
          // the check above should prevent this
          assert(false);
          break;

        case SQL_ERROR:
		  eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to shutdown DriverManager"));
          break;

      }

      henv_ = SQL_NULL_HENV;
    }
  } // lock scope end
}


//static
void DriverManager::_checkInit()
{
  //we lock around this, so two concurrent calls don't result
  //in several HENVs being allocated
  ODBCXX_LOCKER(DMAccess);

  if(henv_==SQL_NULL_HENV)
  {
#if ODBCVER >= 0x0300
    SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&henv_);
#else
    SQLRETURN r = SQLAllocEnv(&henv_);
#endif

    if (r != SQL_SUCCESS && r != SQL_SUCCESS_WITH_INFO)
    {
      throw SQLException
        (ODBCXX_STRING_CONST("Failed to allocate environment handle"), SQLException::scDEFSQLSTATE);
    }

#if ODBCVER >= 0x0300
    // this should immediately follow an AllocEnv per ODBC3
    SQLSetEnvAttr(henv_,
                  SQL_ATTR_ODBC_VERSION,
                  (SQLPOINTER)SQL_OV_ODBC3,
                  SQL_IS_UINTEGER);
#endif

    try
    {
      // don't collect warnings
      eh_ = auto_ptr<ErrorHandler>(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ErrorHandler(false));
    }
    catch (...)
    {
      eh_.reset();
      throw SQLException
        (ODBCXX_STRING_CONST("Failed to allocate error handler"), SQLException::scDEFSQLSTATE);
    }
  }
}



//static
void DriverManager::setLoginTimeout(int to)
{
  ODBCXX_LOCKER(DMAccess);
  loginTimeout_=to;
}

// -------------------------------------------
//static
/** Sets the Driver completion for Connections
 * @param drvcmpl Driver Completion Value
 *	SQL_DRIVER_NOPROMPT, SQL_DRIVER_COMPLETE(Default), SQL_DRIVER_PROMPT, SQL_DRIVER_COMPLETE_REQUIRED
 * See Options for SQLDriverConnect
 */
void DriverManager::setDriverCompletion(SQLUSMALLINT drvcmpl)
{
  ODBCXX_LOCKER(DMAccess);
  DriverManager::driverCompletion_ = drvcmpl;
}
SQLUSMALLINT DriverManager::getDriverCompletion(void)
{ return DriverManager::driverCompletion_;}


//static
int DriverManager::getLoginTimeout()
{
  ODBCXX_LOCKER(DMAccess);
  return loginTimeout_;
}



//static
// This assumes _checkInit has been called
Connection* DriverManager::_createConnection()
{
  Connection* con=0;
  SQLHDBC     hdbc;
  SQLRETURN   r;

  //allocate a handle
#if ODBCVER >= 0x0300
  r = SQLAllocHandle(SQL_HANDLE_DBC,henv_,&hdbc);
#else
  r = SQLAllocConnect(henv_,&hdbc);
#endif

  try
  {
    eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to allocate connection handle"), SQLException::scDEFSQLSTATE);

    con = ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) Connection(hdbc);
    {
      ODBCXX_LOCKER(DMAccess); //since we read loginTimeout_

      //apply the login timeout. -1 means do-not-touch (the default)
      if (loginTimeout_>-1)
      {
#if ODBCVER < 0x0300
        con->_setUIntegerOption(SQL_LOGIN_TIMEOUT, (SQLUINTEGER)loginTimeout_);
#else
        con->_setUIntegerOption(SQL_ATTR_LOGIN_TIMEOUT, (SQLUINTEGER)loginTimeout_);
#endif
      }
    } // end of lock scope
  }
  catch (SQLException &)
  {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) con;
    con = 0;
    throw;
  }
  catch (...)
  {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) con;
    con = 0;
    throw SQLException
      (ODBCXX_STRING_CONST("Failed to allocate connection handle"), SQLException::scDEFSQLSTATE);
  }

  return con;
}



//static
Connection* DriverManager::getConnection(const ODBCXX_STRING& connectString)
{
  Connection* con = 0;

  DriverManager::_checkInit();

  try
  {
    con = DriverManager::_createConnection();
    con->_connect(connectString, getDriverCompletion());
  }
  catch (...)
  {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) con;
    throw;
  }

  return con;
}


//static
Connection* DriverManager::getConnection(const ODBCXX_STRING& dsn,
                                         const ODBCXX_STRING& user,
                                         const ODBCXX_STRING& password)
{
  Connection* con = 0;

  DriverManager::_checkInit();

  try
  {
    con = DriverManager::_createConnection();
    con->_connect(dsn,user,password);
  }
  catch (...)
  {
    ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) con;
    throw;
  }

  return con;
}


DataSourceList* DriverManager::getDataSources()
{
  DriverManager::_checkInit();

  SQLRETURN r;
  SQLSMALLINT dsnlen,desclen;

  DataSourceList* l = ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataSourceList();

  ODBCXX_CHAR_TYPE dsn[SQL_MAX_DSN_LENGTH+1];
  ODBCXX_CHAR_TYPE desc[256];

  {
    ODBCXX_LOCKER(DMAccess);
    r=SQLDataSources(henv_,
                     SQL_FETCH_FIRST,
                     (ODBCXX_SQLCHAR*)dsn,
                     SQL_MAX_DSN_LENGTH+1,
                     &dsnlen,
                     (ODBCXX_SQLCHAR*)desc,
                     256,
                     &desclen);

    eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to obtain a list of datasources"));

    while(r==SQL_SUCCESS || r==SQL_SUCCESS_WITH_INFO) {
      l->insert(l->end(),ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataSource(dsn,desc));

      r=SQLDataSources(henv_,
                       SQL_FETCH_NEXT,
                       (ODBCXX_SQLCHAR*)dsn,
                       SQL_MAX_DSN_LENGTH+1,
                       &dsnlen,
                       (ODBCXX_SQLCHAR*)desc,
                       256,
                       &desclen);

      eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to obtain a list of datasources"));
    }
  } // lock scope end
  return l;
}

#define MAX_DESC_LEN 64
#define MAX_ATTR_LEN 1024

DriverList* DriverManager::getDrivers()
{
  DriverManager::_checkInit();

  SQLRETURN r;
  DriverList* l=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DriverList();


  ODBCXX_CHAR_TYPE desc[MAX_DESC_LEN];
  ODBCXX_CHAR_TYPE attrs[MAX_ATTR_LEN];

  SQLSMALLINT dlen,alen;

  {
    ODBCXX_LOCKER(DMAccess);
    r=SQLDrivers(henv_,
                 SQL_FETCH_FIRST,
                 (ODBCXX_SQLCHAR*)desc,
                 MAX_DESC_LEN,
                 &dlen,
                 (ODBCXX_SQLCHAR*)attrs,
                 MAX_ATTR_LEN,
                 &alen);

    eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to obtain a list of drivers"));

    while(r==SQL_SUCCESS || r==SQL_SUCCESS_WITH_INFO) {
      vector<ODBCXX_STRING> attr;
      unsigned int i=0, last=0;

      //find our attributes
      if(attrs[0]!=0) {
        do {
          while(attrs[++i]!=0);
          attr.push_back(ODBCXX_STRING_CL(&(attrs[last]),i-last));
          last=i+1;
        } while(attrs[last]!=0);
      }

      Driver* d=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) Driver(desc,attr);
      l->insert(l->end(),d);

      r=SQLDrivers(henv_,
                   SQL_FETCH_NEXT,
                   (ODBCXX_SQLCHAR*)desc,
                   MAX_DESC_LEN,
                   &dlen,
                   (ODBCXX_SQLCHAR*)attrs,
                   MAX_ATTR_LEN,
                   &alen);

      eh_->_checkEnvError(henv_,r,ODBCXX_STRING_CONST("Failed to obtain a list of drivers"));
    }
  } // lock scope end

  return l;
}
