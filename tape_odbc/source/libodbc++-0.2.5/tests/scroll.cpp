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
  This should work with almost any almost-compliant database out there,
  providing it supports scrollable cursors.
 */


#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include <iostream>
#include <memory>

using namespace odbc;
using namespace std;

#if !defined(ODBCXX_QT)
# include <sstream>
#else

# undef ASSERT

basic_ostream<ODBCXX_CHAR_TYPE> ostream& operator<<(basic_ostream<ODBCXX_CHAR_TYPE>& o,
                                                    const QString& s)
{
  o << ODBCXX_STRING_CSTR(s);
  return o;
}
#endif

static int assertionsFailed=0;

#define ASSERT(x)                                              \
do {                                                           \
  if(!(x)) {                                                   \
    ODBCXX_CERR << ODBCXX_STRING_CONST("Assertion \"") << #x   \
                << ODBCXX_STRING_CONST("\" failed") << endl;   \
    assertionsFailed++;                                        \
  }                                                            \
} while(false)

#define NAME_PREFIX ODBCXX_STRING_CONST("odbcxx_")

#define TABLE_NAME NAME_PREFIX ODBCXX_STRING_CONST("test")

#define TABLE_ROWS 1000

static void commit(Connection* con)
{
  if(con->getMetaData()->supportsTransactions()) {
    con->commit();
  }
}

static void createStuff(Connection* con)
{
  // create our table
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement());
  stmt->executeUpdate
    (ODBCXX_STRING_CONST("create table ") TABLE_NAME ODBCXX_STRING_CONST("(")
     ODBCXX_STRING_CONST("id integer not null primary key, ")
     ODBCXX_STRING_CONST("name varchar(40) not null)"));

  ODBCXX_COUT << ODBCXX_STRING_CONST("Table ") << TABLE_NAME
              << ODBCXX_STRING_CONST(" created.") << endl;
}

// Drops the database objects.

static void dropStuff(Connection* con)
{
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement());
  try {
    stmt->executeUpdate(ODBCXX_STRING_CONST("drop table ") TABLE_NAME);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Dropped table ") << TABLE_NAME << endl;
  } catch(SQLException& e) {
  }
}

static const ODBCXX_STRING makeName(int n)
{
#if !defined(ODBCXX_QT)
  basic_ostringstream<ODBCXX_CHAR_TYPE> ss;
  ss << ODBCXX_STRING_CONST("This is row number ") << n;
  return ss.str();
#else
  QString s(ODBCXX_STRING_CONST("This is row number "));
  s+=QString::number(n);
  return s;
#endif
}


static void populate(Connection* con)
{
  {
    std::auto_ptr<PreparedStatement> pstmt
      =std::auto_ptr<PreparedStatement>(con->prepareStatement
      (ODBCXX_STRING_CONST("insert into ") TABLE_NAME
       ODBCXX_STRING_CONST(" (id,name) values(?,?)")));
    for(int i=0; i<TABLE_ROWS; i++) {
      pstmt->setInt(1,i);
      pstmt->setString(2,makeName(i));
      pstmt->executeUpdate();
    }
    commit(con);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Inserted ")
                << TABLE_ROWS << ODBCXX_STRING_CONST(" rows.") << endl;
  }
}


static void compare(Connection* con)
{
  // decide whether we should use a scroll insensitive
  // or a scroll sensitive cursor

  int rstype;
  int rsconc;
  DatabaseMetaData* md=con->getMetaData();

  if(md->supportsResultSetType(ResultSet::TYPE_SCROLL_INSENSITIVE)) {
    rstype=ResultSet::TYPE_SCROLL_INSENSITIVE;
  } else if(md->supportsResultSetType(ResultSet::TYPE_SCROLL_SENSITIVE)) {
    rstype=ResultSet::TYPE_SCROLL_SENSITIVE;
  } else {
    ODBCXX_COUT << ODBCXX_STRING_CONST("Skipping compare, data source does ")
                   ODBCXX_STRING_CONST("not support scrollable cursors")
                << endl;
    return;
  }


  if(md->supportsResultSetConcurrency(rstype,ResultSet::CONCUR_READ_ONLY)) {
    // this is all we need
    rsconc=ResultSet::CONCUR_READ_ONLY;
  } else {
    rsconc=ResultSet::CONCUR_UPDATABLE;
  }

  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement
    (rstype,rsconc));
  std::auto_ptr<ResultSet> rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,name from ") TABLE_NAME));

  ASSERT(rs->isBeforeFirst());
  ASSERT(rs->first());
  ASSERT(!rs->isBeforeFirst());
  ASSERT(rs->isFirst());

  ASSERT(rs->last());
  ASSERT(rs->isLast());
  ASSERT(!rs->isAfterLast());
  rs->afterLast();
  ASSERT(rs->isAfterLast());

  ASSERT(rs->previous());
  ASSERT(rs->isLast());


  ODBCXX_COUT << ODBCXX_STRING_CONST("Positioned on the last row (")
              << rs->getRow() << ODBCXX_STRING_CONST(")") << endl;
  int i=TABLE_ROWS;

  do {
    i--;
    ODBCXX_STRING name(makeName(i));
    ASSERT(rs->getInt(1) == i);
    ASSERT(rs->getString(2)==name);
  } while(rs->previous());
  ASSERT(i==0);
  ASSERT(rs->isBeforeFirst());
  ODBCXX_COUT << TABLE_ROWS
              << ODBCXX_STRING_CONST(" rows checked with expected values.")
              << endl;
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

    // we don't want autocommit
    if(con->getMetaData()->supportsTransactions()) {
      con->setAutoCommit(false);
    }
//      con->setTraceFile("/tmp/fisk");
//      con->setTrace(true);

    dropStuff(con.get());
    createStuff(con.get());

    populate(con.get());
    compare(con.get());
    commit(con.get());

    dropStuff(con.get());


    commit(con.get());

    if(assertionsFailed>0) {
      ODBCXX_COUT << assertionsFailed
                  << ODBCXX_STRING_CONST(" assertions failed.") << endl;
    }
  } catch(SQLException& e) {
    ODBCXX_CERR << endl << e.getMessage() << endl;
    return 1;
  }

  return 0;
}
