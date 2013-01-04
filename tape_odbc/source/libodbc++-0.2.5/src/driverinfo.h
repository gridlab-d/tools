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

#ifndef __DRIVERINFO_H
#define __DRIVERINFO_H

#include <odbc++/types.h>

namespace odbc {

  class Connection;

  class ODBCXX_EXPORT DriverInfo {
    friend class Connection;
  public:
    int getMajorVersion() const {
      return majorVersion_;
    }

    int getMinorVersion() const {
      return minorVersion_;
    }

    int getCursorMask() const {
      return cursorMask_;
    }

    bool supportsForwardOnly() const {
      return (cursorMask_&SQL_SO_FORWARD_ONLY)!=0;
    }

    bool supportsStatic() const {
      return (cursorMask_&SQL_SO_STATIC)!=0;
    }

    bool supportsKeyset() const {
      return (cursorMask_&SQL_SO_KEYSET_DRIVEN)!=0;
    }

    bool supportsDynamic() const {
      return (cursorMask_&SQL_SO_DYNAMIC)!=0;
    }

    bool supportsScrollSensitive() const {
      return
        this->supportsDynamic() ||
        this->supportsKeyset();
    }

    // assumes that this->supportsScrollSensitive()==true
    int getScrollSensitive() const {
      if(this->supportsDynamic()) {
        return SQL_CURSOR_DYNAMIC;
      } else {
        return SQL_CURSOR_KEYSET_DRIVEN;
      }
    }

    // concurrency, ct is an SQL_CURSOR constant
    bool supportsReadOnly(int ct) const;
    bool supportsLock(int ct) const;
    bool supportsRowver(int ct) const;
    bool supportsValues(int ct) const;

    bool supportsUpdatable(int ct) const {
      return
        this->supportsLock(ct) ||
        this->supportsRowver(ct) ||
        this->supportsValues(ct);
    }

    int getUpdatable(int ct) const {
      // assumes supportsUpdatable(ct) returns true
      if(this->supportsRowver(ct)) {
        return SQL_CONCUR_ROWVER;
      } else if(this->supportsValues(ct)) {
        return SQL_CONCUR_VALUES;
      } else if(this->supportsLock(ct)) {
        return SQL_CONCUR_LOCK;
      }
      return SQL_CONCUR_READ_ONLY;
    }
        // GetDataSupport
        bool supportsGetDataAnyOrder(void) const
        {  return (getdataExt_ & SQL_GD_ANY_ORDER)!=0;  };
        bool supportsGetDataAnyColumn(void) const
        {  return (getdataExt_ & SQL_GD_ANY_COLUMN)!=0; };
        bool supportsGetDataBlock(void) const
        {  return (getdataExt_ & SQL_GD_BLOCK)!=0;      };
        bool supportsGetDataBound(void) const
        {  return (getdataExt_ & SQL_GD_BOUND)!=0;      };
        //
    bool supportsFunction(int funcId) const {
      return ODBC3_C
        (SQL_FUNC_EXISTS(supportedFunctions_,funcId),
         supportedFunctions_[funcId])==SQL_TRUE;
    }

  private:
    // odbc version
    int majorVersion_;
    int minorVersion_;

    int getdataExt_;
    int cursorMask_;
#if ODBCVER >= 0x0300
    int forwardOnlyA2_;
    int staticA2_;
    int keysetA2_;
    int dynamicA2_;
#endif
    int concurMask_;


    SQLUSMALLINT* supportedFunctions_;

    explicit DriverInfo(Connection* con);
    ~DriverInfo() {
      delete[] supportedFunctions_;
    }
  };

} // namespace odbc


#endif // __DRIVERINFO_H

