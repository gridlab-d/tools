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

// this will probably include more info about the driver in the
// future

#include "driverinfo.h"
#include <odbc++/connection.h>
#include <odbc++/databasemetadata.h>

using namespace odbc;

DriverInfo::DriverInfo(Connection* con)
  :concurMask_(0),
#if ODBCVER >= 0x0300
   forwardOnlyA2_(0),
   staticA2_(0),
   keysetA2_(0),
   dynamicA2_(0),
#endif
  supportedFunctions_(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) SQLUSMALLINT[ODBC3_C(SQL_API_ODBC3_ALL_FUNCTIONS_SIZE,
						100)])
{
  DatabaseMetaData* md=con->getMetaData();
  majorVersion_=md->getDriverMajorVersion();
  minorVersion_=md->getDriverMinorVersion();

// Load getDataExtention capabilities
  getdataExt_=md->_getNumeric32(SQL_GETDATA_EXTENSIONS);
  cursorMask_=md->_getNumeric32(SQL_SCROLL_OPTIONS);

#if ODBCVER >= 0x0300
  if(majorVersion_>=3) {
    if(cursorMask_&SQL_SO_FORWARD_ONLY) {
      forwardOnlyA2_=md->_getNumeric32(SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2);
    }
    if(cursorMask_&SQL_SO_STATIC) {
      staticA2_=md->_getNumeric32(SQL_STATIC_CURSOR_ATTRIBUTES2);
    }
    if(cursorMask_&SQL_SO_KEYSET_DRIVEN) {
      keysetA2_=md->_getNumeric32(SQL_KEYSET_CURSOR_ATTRIBUTES2);
    }
    if(cursorMask_&SQL_SO_DYNAMIC) {
      dynamicA2_=md->_getNumeric32(SQL_DYNAMIC_CURSOR_ATTRIBUTES2);
    }
  } else {
#endif

    // use the ODBC2 way of finding out things
    concurMask_=md->_getNumeric32(SQL_SCROLL_CONCURRENCY);

#if ODBCVER >= 0x0300
  }
#endif


  SQLRETURN r=SQLGetFunctions(con->hdbc_,
                              ODBC3_C(SQL_API_ODBC3_ALL_FUNCTIONS,
                                      SQL_API_ALL_FUNCTIONS),
                              supportedFunctions_);
  con->_checkConError(con->hdbc_,
                      r,ODBCXX_STRING_CONST("Failed to retrieve a list of supported functions"));
}



#if ODBCVER >= 0x0300

#define IMPLEMENT_SUPPORTS(SUFFIX,VALUE_3,VALUE_2)   \
bool DriverInfo::supports##SUFFIX(int ct) const      \
{                                                    \
  bool r=false;                                      \
                                                     \
  if(majorVersion_>=3) {                             \
    switch(ct) {                                     \
    case SQL_CURSOR_FORWARD_ONLY:                    \
      r=(forwardOnlyA2_&VALUE_3)!=0;                 \
      break;                                         \
                                                     \
    case SQL_CURSOR_STATIC:                          \
      r=(staticA2_&VALUE_3)!=0;                      \
      break;                                         \
                                                     \
    case SQL_CURSOR_KEYSET_DRIVEN:                   \
      r=(keysetA2_&VALUE_3)!=0;                      \
      break;                                         \
                                                     \
    case SQL_CURSOR_DYNAMIC:                         \
      r=(dynamicA2_&VALUE_3)!=0;                     \
      break;                                         \
                                                     \
    default:                                         \
      assert(false);                                 \
      break;                                         \
    }                                                \
  } else {                                           \
    r=(concurMask_&VALUE_2)!=0;                      \
  }                                                  \
  return r;                                          \
}

#else

#define IMPLEMENT_SUPPORTS(SUFFIX,VALUE_3,VALUE_2)   \
bool DriverInfo::supports##SUFFIX(int ct) const      \
{                                                    \
  return (concurMask_&VALUE_2)!=0;                   \
}

#endif


IMPLEMENT_SUPPORTS(ReadOnly,SQL_CA2_READ_ONLY_CONCURRENCY,SQL_SCCO_READ_ONLY)
IMPLEMENT_SUPPORTS(Lock,SQL_CA2_LOCK_CONCURRENCY,SQL_SCCO_LOCK)
IMPLEMENT_SUPPORTS(Rowver,SQL_CA2_OPT_ROWVER_CONCURRENCY,SQL_SCCO_OPT_ROWVER)
IMPLEMENT_SUPPORTS(Values,SQL_CA2_OPT_VALUES_CONCURRENCY,SQL_SCCO_OPT_VALUES)

