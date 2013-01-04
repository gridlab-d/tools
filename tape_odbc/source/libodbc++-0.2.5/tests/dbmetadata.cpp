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
#include <odbc++/connection.h>
#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>
#include <odbc++/databasemetadata.h>

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


inline const ODBCXX_CHAR_TYPE* s(bool b)
{
  return b?ODBCXX_STRING_CONST("Yes"):ODBCXX_STRING_CONST("No");
}

inline ODBCXX_STRING maybeQuote(const ODBCXX_STRING& str, const ODBCXX_STRING& qc)
{
  if(qc!=ODBCXX_STRING_CONST(" ")) {
    return qc+str+qc;
  }
  return str;
}

static void rsInfo(DatabaseMetaData* md, int rsType, const ODBCXX_CHAR_TYPE* name)
{
  if(md->supportsResultSetType(rsType)) {
      ODBCXX_COUT << name << endl;

      if(md->supportsResultSetConcurrency(rsType,
                                          ResultSet::CONCUR_READ_ONLY)) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("  + ResultSet::CONCUR_READ_ONLY") << endl;
      }

      if(md->supportsResultSetConcurrency(rsType,
                                          ResultSet::CONCUR_UPDATABLE)) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("  + ResultSet::CONCUR_UPDATABLE") << endl;
      }

      if(md->ownInsertsAreVisible(rsType)) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("    Own inserts are visible") << endl;
      }

      if(md->ownUpdatesAreVisible(rsType)) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("    Own updates are visible") << endl;
      }
      if(md->ownDeletesAreVisible(rsType)) {
        ODBCXX_COUT << ODBCXX_STRING_CONST("    Own deletes are visible") << endl;
      }

      ODBCXX_COUT << endl;
  }
}


void transactionInfo(DatabaseMetaData* md)
{
  ODBCXX_COUT << ODBCXX_STRING_CONST("Transaction support") << endl
              << ODBCXX_STRING_CONST("===================") << endl;

  if(!md->supportsTransactions()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("This datasource does ")
                   ODBCXX_STRING_CONST("not support transactions.")
                << endl << endl;
    return;
  }

  int defIsolation=md->getDefaultTransactionIsolation();

  static struct {
    int id;
    const ODBCXX_CHAR_TYPE* name;
  } levels[] = {
    { Connection::TRANSACTION_READ_UNCOMMITTED,
      ODBCXX_STRING_CONST("Connection::TRANSACTION_READ_UNCOMMITTED") },
    { Connection::TRANSACTION_READ_COMMITTED,
      ODBCXX_STRING_CONST("Connection::TRANSACTION_READ_COMMITTED") },
    { Connection::TRANSACTION_REPEATABLE_READ,
      ODBCXX_STRING_CONST("Connection::TRANSACTION_REPEATABLE_READ") },
    { Connection::TRANSACTION_SERIALIZABLE,
      ODBCXX_STRING_CONST("Connection::TRANSACTION_SERIALIZABLE") },
    { 0,NULL }
  };

  for(int i=0; levels[i].name!=NULL; i++) {
    if(md->supportsTransactionIsolationLevel(levels[i].id)) {
      ODBCXX_COUT << ODBCXX_STRING_CONST(" +") << levels[i].name
                  << (levels[i].id==defIsolation
                     ?ODBCXX_STRING_CONST(" (default)")
                     :ODBCXX_STRING_CONST(""))
                  << endl;
    }
  }

  // the longest method name I've ever seen!
  if(md->supportsDataDefinitionAndDataManipulationTransactions()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("  Both DML and DDL can be used within a transaction") << endl;
  } else if(md->supportsDataManipulationTransactionsOnly()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("  Only DML can be used within a transaction") << endl;
  } else if(md->dataDefinitionCausesTransactionCommit()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("  DDL causes commit") << endl;
  } else if(md->dataDefinitionIgnoredInTransactions()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("  DDL is ignored in transactions") << endl;
  }

  ODBCXX_COUT << endl;
}


int main(int argc, char** argv)
{
  if(argc!=2 && argc!=4) {
    cerr << "Usage: " << argv[0] << " connect-string" << endl
         << "or     " << argv[0] << " dsn username password" << endl;
    return 0;
  }
  try {
    std::vector<ODBCXX_STRING> vargv(argc-1);
    const size_t MAX_CHARS = 256;
    for(int i=1;i<argc;++i)
    {
      ODBCXX_STRING& arg=vargv[i-1];
#if defined(ODBCXX_UNICODE)
      wchar_t buffer[MAX_CHARS];
      size_t len=mbstowcs(buffer,argv[i],MAX_CHARS);
      if(0<len&&MAX_CHARS>len)
      {
         arg=buffer;
      }
#else
      arg=argv[i];
#endif
    }
    std::auto_ptr<Connection> con;
    if(argc==2) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Connecting to ") << vargv[0]
                  << ODBCXX_STRING_CONST("...") << flush;
      con=std::auto_ptr<Connection>(DriverManager::getConnection(vargv[0]));
    } else {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Connecting to dsn=") << vargv[0]
                  << ODBCXX_STRING_CONST(", uid=") << vargv[1]
                  << ODBCXX_STRING_CONST(", pwd=") << vargv[2]
                  << ODBCXX_STRING_CONST("...") << flush;
      con=std::auto_ptr<Connection>(DriverManager::getConnection(vargv[0],vargv[1],vargv[2]));
    }
    ODBCXX_COUT << ODBCXX_STRING_CONST(" done.") << endl;

    DatabaseMetaData* md=con->getMetaData();

    ODBCXX_COUT << ODBCXX_STRING_CONST("Product name                    : ")
                << md->getDatabaseProductName() << endl

                << ODBCXX_STRING_CONST("Product version                 : ")
                << md->getDatabaseProductVersion() << endl

                << ODBCXX_STRING_CONST("Driver name                     : ")
                << md->getDriverName() << endl

                << ODBCXX_STRING_CONST("Driver version                  : ")
                << md->getDriverVersion() << endl

                << ODBCXX_STRING_CONST("Driver ODBC version             : ")
                << md->getDriverMajorVersion() << ODBCXX_STRING_CONST(".")
                << md->getDriverMinorVersion() << endl

                << ODBCXX_STRING_CONST("Supports transactions           : ")
                << s(md->supportsTransactions()) << endl;

    ODBCXX_COUT << endl;

    transactionInfo(md);


    ODBCXX_COUT << ODBCXX_STRING_CONST("Supported system functions") << endl
                << ODBCXX_STRING_CONST("==========================") << endl
                << md->getSystemFunctions() << endl << endl
                << ODBCXX_STRING_CONST("Supported string functions") << endl
                << ODBCXX_STRING_CONST("==========================") << endl
                << md->getStringFunctions() << endl << endl
                << ODBCXX_STRING_CONST("Supported time/date functions") << endl
                << ODBCXX_STRING_CONST("=============================") << endl
                << md->getTimeDateFunctions() << endl << endl
                << ODBCXX_STRING_CONST("Supported numeric functions") << endl
                << ODBCXX_STRING_CONST("===========================") << endl
                << md->getNumericFunctions() << endl
                << ODBCXX_STRING_CONST("Non-ODBC SQL keywords") << endl
                << ODBCXX_STRING_CONST("=====================") << endl
                << md->getSQLKeywords() << endl;

    ODBCXX_COUT << endl;



    ODBCXX_COUT << ODBCXX_STRING_CONST("Supported ResultSet types") << endl
                << ODBCXX_STRING_CONST("=========================") << endl;

    rsInfo(md,ResultSet::TYPE_FORWARD_ONLY,
           ODBCXX_STRING_CONST("ResultSet::TYPE_FORWARD_ONLY"));
    rsInfo(md,ResultSet::TYPE_SCROLL_INSENSITIVE,
           ODBCXX_STRING_CONST("ResultSet::TYPE_SCROLL_INSENSITIVE"));
    rsInfo(md,ResultSet::TYPE_SCROLL_SENSITIVE,
           ODBCXX_STRING_CONST("ResultSet::TYPE_SCROLL_SENSITIVE"));


    bool cdml=md->supportsCatalogsInDataManipulation();
    bool cproc=md->supportsCatalogsInProcedureCalls();
    bool ctd=md->supportsCatalogsInTableDefinitions();
    bool cid=md->supportsCatalogsInIndexDefinitions();
    bool cpd=md->supportsCatalogsInPrivilegeDefinitions();

    bool sdml=md->supportsSchemasInDataManipulation();
    bool sproc=md->supportsSchemasInProcedureCalls();
    bool std=md->supportsSchemasInTableDefinitions();
    bool sid=md->supportsSchemasInIndexDefinitions();
    bool spd=md->supportsSchemasInPrivilegeDefinitions();

    bool hasCatalogs=cdml || cproc || ctd || cid || cpd;

    if(hasCatalogs) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Supported catalog uses") << endl
                  << ODBCXX_STRING_CONST("======================") << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Data manipulation    : ") << s(cdml) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Procedure calls      : ") << s(cproc) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Table definitions    : ") << s(ctd) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Index definitions    : ") << s(cid) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Privilege definitions: ") << s(cpd) << endl;
    } else {
      ODBCXX_COUT << ODBCXX_STRING_CONST("This datasource does not support catalogs") << endl;
    }
    ODBCXX_COUT << endl;

    bool hasSchemas=sdml || sproc || std || sid || spd;

    if(hasSchemas) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Supported schema uses") << endl
                  << ODBCXX_STRING_CONST("=====================") << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Data manipulation    : ") << s(sdml) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Procedure calls      : ") << s(sproc) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Table definitions    : ") << s(std) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Index definitions    : ") << s(sid) << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Privilege definitions: ") << s(spd) << endl;
    } else {
      ODBCXX_COUT << ODBCXX_STRING_CONST("This datasource does not support schemas") << endl;
    }
    ODBCXX_COUT << endl;

    ODBCXX_STRING idq(md->getIdentifierQuoteString());
    // space means no quoting supported

    ODBCXX_STRING id(maybeQuote(md->getTableTerm(),idq));
    if(hasSchemas) {
      id=maybeQuote(md->getSchemaTerm(),idq)+ODBCXX_STRING_CONST(".")+id;
    }
    if(hasCatalogs) {
      ODBCXX_STRING catSep(md->getCatalogSeparator());
      if(md->isCatalogAtStart()) {
        id=maybeQuote(md->getCatalogTerm(),idq)+catSep+id;
      } else {
        id+=catSep+maybeQuote(md->getCatalogTerm(),idq);
      }
    }

    ODBCXX_COUT << ODBCXX_STRING_CONST("Preferred table identifier format: ") << id << endl;

    if(hasCatalogs) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Tables available: ") << endl;
      std::vector<ODBCXX_STRING> types;
      auto_ptr<odbc::ResultSet> rs(md->getTables(ODBCXX_STRING(),
                                                ODBCXX_STRING(),
                                                ODBCXX_STRING(),
                                                types));
      while(rs->next()) {
        ODBCXX_COUT << rs->getString(1) << ODBCXX_STRING_CONST(".")
             << rs->getString(2) << ODBCXX_STRING_CONST(".")
             << rs->getString(3) << ODBCXX_STRING_CONST(" type=")
             << rs->getString(4) << ODBCXX_STRING_CONST(" remarks=")
             << rs->getString(5) <<endl;
      }
    }

  } catch(SQLException& e) {
    ODBCXX_CERR << endl << e.getMessage() << endl;
  }

  return 0;
}
