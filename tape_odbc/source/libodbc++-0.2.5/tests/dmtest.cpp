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

#include <iostream>
#include <memory>

using namespace odbc;
using namespace std;

#if defined(ODBCXX_QT)
basic_ostream<ODBCXX_CHAR_TYPE>& operator<<(basic_ostream<ODBCXX_CHAR_TYPE>& o,
                                            const QString& s)
{
  o << ODBCXX_STRING_CSTR(s);
  return o;
}
#endif

int main(void)
{
  try {
    ODBCXX_COUT << ODBCXX_STRING_CONST("Available datasources: ") << endl;

    std::auto_ptr<DataSourceList> l=
      std::auto_ptr<DataSourceList>(DriverManager::getDataSources());

    for(DataSourceList::iterator i=l->begin();
        i!=l->end(); i++) {
      DataSource* ds=(*i);
      ODBCXX_COUT << ODBCXX_STRING_CONST("\t") << ds->getName();

      if(ds->getDescription().length()>0) {
        ODBCXX_COUT << ODBCXX_STRING_CONST(" (") << ds->getDescription()
                    << ODBCXX_STRING_CONST(")");
      }

      ODBCXX_COUT << endl;
    }

    ODBCXX_COUT << ODBCXX_STRING_CONST("Available drivers: ") << endl;

    std::auto_ptr<DriverList> dl=std::auto_ptr<DriverList>(DriverManager::getDrivers());

    for(DriverList::iterator j=dl->begin();
        j!=dl->end(); j++) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("\t") << (*j)->getDescription() << endl;

      const vector<ODBCXX_STRING>& attrs=(*j)->getAttributes();
      for(vector<ODBCXX_STRING>::const_iterator x=attrs.begin(); x!=attrs.end(); x++) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("\t\tAttribute: ") << *x << endl;
      }

      ODBCXX_COUT << endl;
    }
  } catch(SQLException& e) {
    ODBCXX_CERR << endl << e.getMessage() << endl;
  }

  return 0;
}
