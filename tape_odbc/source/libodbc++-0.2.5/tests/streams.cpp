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
  This tests the ResultSet's (get|update)(Ascii|Binary)Stream.
  Expected to be run against an Oracle database using Openlink's driver
  You must have create table/drop table rights.

  This test does the following:
  1. Create a table containing a number column and a long raw column.
  2. Insert some rows into the table using a PreparedStatement
  3. Insert some more rows into the table using a ResultSet
  4. Compare the contents of the table with values from the
  value generator.
  5. Update all the rows in the table using a ResultSet
  6. Compare again.
  7. Drop table

  The value generator simply gives different binary chunks of different sizes
  depending on the row number.

  This depends on a datasource that keeps statements open across
  commits.
 */

#if !defined(ODBCXX_QT)

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include <iostream>
#include <sstream>
#include <memory>
#include <stdlib.h>

using namespace odbc;
using namespace std;

#define NAME_PREFIX ODBCXX_STRING_CONST("odbcxx_")

#define TABLE_NAME NAME_PREFIX ODBCXX_STRING_CONST("test")

const int SIZE_MULTIPLIER = 1024*20; //1st row=20k, 2nd row=40k ...
const int MAX_SIZE        = 1024*1024; //max 1 megabyte per value
const int TEST_ROWS       = 20; //

typedef pair<ODBCXX_STREAM*,int> TestStream;


static int assertionsFailed=0;

#define ASSERT(x)                                                \
do {                                                             \
  if(!(x)) {                                                     \
    cerr << "Assertion \"" << #x << "\" failed" << endl;         \
    assertionsFailed++;                                          \
  }                                                              \
} while(false)



// dumps all warnings for a given ErrorHandler
static void dumpWarnings(ErrorHandler* eh, const ODBCXX_STRING& who)
{
  WarningList* warnings=eh->getWarnings();
  if(!warnings->empty()) {
    ODBCXX_COUT << who << ODBCXX_STRING_CONST(" had ")
                << warnings->size() << ODBCXX_STRING_CONST(" warnings.") << endl;
    for(WarningList::iterator i=warnings->begin();
        i!=warnings->end(); i++) {
      SQLWarning& w=**i;
      ODBCXX_COUT << ODBCXX_STRING_CONST("SQLState   : ") << w.getSQLState() << endl;
      ODBCXX_COUT << ODBCXX_STRING_CONST("Description: ") << w.getMessage()  << endl;
    }
  }
}


// Creates the database objects needed

static void createStuff(Connection* con)
{
  // create our table
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement());
  stmt->executeUpdate(
    ODBCXX_STRING_CONST("create table ") TABLE_NAME ODBCXX_STRING_CONST("(")
    ODBCXX_STRING_CONST("id number constraint ") TABLE_NAME
    ODBCXX_STRING_CONST("_pk_id primary key, content long raw not null)"));

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
  } catch(SQLException& /*e*/) {
    //ignore
  }
}


class ValueGen {
private:
  size_t cnt_;
  size_t rows_;

  ODBCXX_CHAR_TYPE current[MAX_SIZE];

public:
  ValueGen(size_t rows)
    :cnt_(0), rows_(rows) {}
  ~ValueGen() {}

  bool next() {
    return ((++cnt_) <= rows_);
  }

  int id() {
    return cnt_;
  }

  TestStream* b() {
    // cnt_ can never be 0 here
    int len=((cnt_)*SIZE_MULTIPLIER)%MAX_SIZE;

    for(int i=0; i<len; i++) {
      current[i]=((i+cnt_)%256);
    }
    basic_istringstream<ODBCXX_CHAR_TYPE>* s
      =new basic_istringstream<ODBCXX_CHAR_TYPE>(current);
    return new TestStream(s,len);
  }
};


// this compares two streams
inline bool compareStreams(ODBCXX_STREAM* s1, ODBCXX_STREAM* s2, int len)
{
  ODBCXX_CHAR_TYPE c1, c2;
  size_t cnt=0;
  while(s1->get(c1) && s2->get(c2)) {
    cnt++;
    if(c1!=c2)
      return false;
  }
  return (cnt==len);
}



static void populate(Connection* con)
{
  std::auto_ptr<PreparedStatement> pstmt
    =std::auto_ptr<PreparedStatement>(con->prepareStatement
    (ODBCXX_STRING_CONST("insert into ") TABLE_NAME
     ODBCXX_STRING_CONST("(id,content) values(?,?)")));

  ValueGen vg(TEST_ROWS);
  int cnt=0;
  while(cnt<TEST_ROWS/2 && vg.next()) {
    pstmt->setInt(1,vg.id());
    TestStream* s=vg.b();
    pstmt->setBinaryStream(2,s->first,s->second);
    pstmt->executeUpdate();

    delete s->first;
    delete s;

    //we commit after every row, since we can reach pretty
    //big chunks of data
    con->commit();
    cnt++;
  }

  cout << "Inserted " << cnt << " rows using a PreparedStatement" << endl;

  // go cursors
  std::auto_ptr<Statement> stmt
    =std::auto_ptr<Statement>(con->createStatement(ResultSet::TYPE_SCROLL_SENSITIVE,
                                                   ResultSet::CONCUR_UPDATABLE));
  std::auto_ptr<ResultSet> rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,content from ") TABLE_NAME));

  rs->next();
  rs->moveToInsertRow();
  while(vg.next()) {
    rs->updateInt(1,vg.id());
    TestStream* s=vg.b();
    rs->updateBinaryStream(2,s->first,s->second);

    rs->insertRow();

    ODBCXX_COUT << ODBCXX_STRING_CONST("Did a row") << endl;

    delete s->first;
    delete s;

    con->commit();
  }

  ODBCXX_COUT << ODBCXX_STRING_CONST("Inserted ") << TEST_ROWS/2
              << ODBCXX_STRING_CONST(" rows using a ResultSet") << endl;
}


static void compare(Connection* con)
{
  std::auto_ptr<Statement> stmt
    =std::auto_ptr<Statement>(con->createStatement());
  std::auto_ptr<ResultSet> rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,content from ") TABLE_NAME
     ODBCXX_STRING_CONST(" order by id")));

  ValueGen vg(TEST_ROWS);
  int cnt=0;

  cout << "Comparing values with database (can take a while)..." << flush;

  while(rs->next() && vg.next()) {
    cnt++;
    ASSERT(vg.id()==rs->getInt(1));

    TestStream* vgStream=vg.b();
    ODBCXX_STREAM* rsStream=rs->getBinaryStream(2);

    ASSERT(compareStreams(vgStream->first,rsStream,vgStream->second));

    delete vgStream->first;
    delete vgStream;

    //rsStream is taken care of by the result set
  }

  ASSERT(cnt==TEST_ROWS);

  cout << " done." << endl;
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
    con->setAutoCommit(false);

    dropStuff(con.get());
    createStuff(con.get());

    populate(con.get());
    compare(con.get());

    dropStuff(con.get());


    con->commit();
  } catch(SQLException& e) {
    ODBCXX_CERR << endl << e.getMessage() << endl;
  }

  return 0;
}

#else

int main() { return 0; }

#endif
