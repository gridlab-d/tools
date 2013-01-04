#if !defined(ODBCXX_QT)

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/callablestatement.h>
#include <odbc++/databasemetadata.h>

#include <sstream>
#include <iostream>
#include <memory>

using namespace std;
using namespace odbc;

// note: this must be even
const int TABLE_ROWS=500;

#define PREFIX ODBCXX_STRING_CONST("odbcxx_")

const ODBCXX_STRING tableName(PREFIX ODBCXX_STRING_CONST("test"));
const ODBCXX_STRING procName(PREFIX ODBCXX_STRING_CONST("ptest"));
const ODBCXX_STRING funcName(PREFIX ODBCXX_STRING_CONST("ftest"));

int assertionsFailed=0;

#define ASSERT(x)                                              \
do {                                                           \
  if(!(x)) {                                                   \
    ODBCXX_CERR << ODBCXX_STRING_CONST("Assertion \"") << #x   \
                << ODBCXX_STRING_CONST("\" failed") << endl;   \
    assertionsFailed++;                                        \
  }                                                            \
} while(false)

static void dumpWarnings(Statement* stmt)
{
  std::auto_ptr<WarningList> warnings
    =std::auto_ptr<WarningList>(stmt->getWarnings());
  for(WarningList::iterator i=warnings->begin();
      i!=warnings->end(); i++) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("Warning: ")
                << (*i)->getMessage() << endl;
  }
}

static void dropStuff(Connection* con)
{
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement());
  try {
    stmt->executeUpdate(ODBCXX_STRING_CONST("drop table ")+tableName);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Dropped table ") << tableName << endl;
    dumpWarnings(stmt.get());
  } catch(SQLException& e) {}

  try {
    stmt->executeUpdate(ODBCXX_STRING_CONST("drop procedure ")+procName);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Dropped procedure ") << procName << endl;
    dumpWarnings(stmt.get());
  } catch(SQLException& e) {}

  try {
    stmt->executeUpdate(ODBCXX_STRING_CONST("drop function ")+funcName);
    ODBCXX_COUT << ODBCXX_STRING_CONST("Dropped function ") << funcName << endl;
    dumpWarnings(stmt.get());
  } catch(SQLException& e) {}
}

static void createStuff(Connection* con)
{
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement());

  // create the table
  stmt->executeUpdate(ODBCXX_STRING_CONST("create table ")+tableName+
                      ODBCXX_STRING_CONST(" (id number(4) not null primary key, ")
                      ODBCXX_STRING_CONST("name varchar2(22) not null, ")
                      ODBCXX_STRING_CONST("ts date)"));

  dumpWarnings(stmt.get());
  ODBCXX_COUT << ODBCXX_STRING_CONST("Created table ") << tableName << endl;

  // create the procedure
  stmt->executeUpdate
    (ODBCXX_STRING_CONST("create procedure ")+procName+
     ODBCXX_STRING_CONST(" (a in integer, b out integer, s in out varchar2) as ")
     ODBCXX_STRING_CONST("begin ")
     ODBCXX_STRING_CONST("  b:=a*2; ")
     ODBCXX_STRING_CONST("  s:= s || ': ' || a || '*2=' || b; ")
     ODBCXX_STRING_CONST("end;"));

  dumpWarnings(stmt.get());
  ODBCXX_COUT << ODBCXX_STRING_CONST("Created procedure ") << procName << endl;

  // create the function
  stmt->executeUpdate
    (ODBCXX_STRING_CONST("create function ")+funcName+
     ODBCXX_STRING_CONST(" (a in number, s in out varchar2) ")
     ODBCXX_STRING_CONST("return number as ")
     ODBCXX_STRING_CONST("b number; ")
     ODBCXX_STRING_CONST("begin ")
     ODBCXX_STRING_CONST("  b:=a*2; ")
     ODBCXX_STRING_CONST("  s:= s || ': ' || a || '*2=' || b; ")
     ODBCXX_STRING_CONST("  return b; ")
     ODBCXX_STRING_CONST("end;"));

  dumpWarnings(stmt.get());
  ODBCXX_COUT << ODBCXX_STRING_CONST("Created function ") << funcName << endl;
}

static void testProc(Connection* con)
{
  std::auto_ptr<CallableStatement> stmt=std::auto_ptr<CallableStatement>(con->prepareCall
    (ODBCXX_STRING_CONST("{call ")+procName+ODBCXX_STRING_CONST("(?,?,?)}")));
  stmt->setInt(1,22);
  stmt->registerOutParameter(2,Types::INTEGER);
  stmt->setString(3, ODBCXX_STRING_CONST("Okay"));
  stmt->executeUpdate();

  ASSERT(stmt->getInt(2)==44);
  ASSERT(stmt->getString(3)==ODBCXX_STRING_CONST("Okay: 22*2=44"));
}

static void testFunc(Connection* con)
{
  std::auto_ptr<CallableStatement> stmt=std::auto_ptr<CallableStatement>(con->prepareCall
    (ODBCXX_STRING_CONST("{?=call ")+procName+ODBCXX_STRING_CONST("(?,?)}")));

  stmt->registerOutParameter(1,Types::INTEGER);
  stmt->setInt(2,22);
  stmt->setString(3,ODBCXX_STRING_CONST("Okay"));
  stmt->executeUpdate();

  ASSERT(stmt->getInt(1)==44);
  ASSERT(stmt->getString(3)==ODBCXX_STRING_CONST("Okay: 22*2=44"));
}


static void testTable(Connection* con)
{
  int i=0;
  int driverVersion=con->getMetaData()->getDriverMajorVersion();

  if(driverVersion<3) {
    // insert the first row using a prepared statement
    // ODBC 2 drivers can't do inserts before a fetch is done.
    // some can't do inserts if the result set is not on a
    // real row.
    std::auto_ptr<PreparedStatement> pstmt
      =std::auto_ptr<PreparedStatement>(con->prepareStatement
      (ODBCXX_STRING_CONST("insert into ")+tableName+
       ODBCXX_STRING_CONST(" (id,name,ts) values(?,?,?)")));
    pstmt->setInt(1,i);
    pstmt->setString(2,ODBCXX_STRING_CONST("This is row number 0"));
    {
      Timestamp ts;
      pstmt->setTimestamp(3,ts);
    }
    pstmt->executeUpdate();
    ODBCXX_COUT << ODBCXX_STRING_CONST("Inserted row 0") << endl;
    i++;
  }

  // populate our table using a ResultSet
  std::auto_ptr<Statement> stmt=std::auto_ptr<Statement>(con->createStatement
    (ResultSet::TYPE_SCROLL_SENSITIVE, ResultSet::CONCUR_UPDATABLE));

  // set fetch size to something useful
  stmt->setFetchSize(10);

  std::auto_ptr<ResultSet> rs=std::auto_ptr<ResultSet>
    (stmt->executeQuery(ODBCXX_STRING_CONST("select id,name,ts from ")+tableName));

  if(driverVersion<3) {
    // position ourselves on a real row
    ASSERT(rs->next());
  }

  rs->moveToInsertRow();

  while(i<TABLE_ROWS) {
    basic_ostringstream<ODBCXX_CHAR_TYPE> ns;
    ns << ODBCXX_STRING_CONST("This is row number ") << i;
    ODBCXX_STRING name(ns.str());
    rs->updateInt(1,i);
    rs->updateString(2,name);
    rs->updateTimestamp(3,Timestamp());
    rs->insertRow();
    ODBCXX_COUT << ODBCXX_STRING_CONST("Inserted row ") << i << endl;
    i++;
  }
  rs->moveToCurrentRow();

  con->commit();

  rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,name from ")+tableName));

  i=0;
  while(rs->next()) {
    ODBCXX_COUT << ODBCXX_STRING_CONST("Checking row ") << i << endl;
    basic_ostringstream<ODBCXX_CHAR_TYPE> ns;
    ns << ODBCXX_STRING_CONST("This is row number ") << i;
    ODBCXX_STRING name(ns.str());
    ASSERT(rs->getString(ODBCXX_STRING_CONST("name"))==name);

    ASSERT(rs->getInt(ODBCXX_STRING_CONST("id"))==i);
    i++;
  }

  ODBCXX_COUT << ODBCXX_STRING_CONST("Check done") << endl;

  rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,name,ts from ")+tableName));
  i=0;
  while(rs->next()) {
    if((i%2)==1) {
      basic_ostringstream<ODBCXX_CHAR_TYPE> ns;
      ns << ODBCXX_STRING_CONST("This IS row number ") << i;
      ODBCXX_STRING name(ns.str());

      rs->updateString(ODBCXX_STRING_CONST("name"),name);
      Timestamp ts;
      rs->updateTimestamp(ODBCXX_STRING_CONST("ts"),ts);
      rs->updateRow();
      ODBCXX_COUT << ODBCXX_STRING_CONST("Updated row ") << i << endl;
    } else {
      rs->deleteRow();
      ODBCXX_COUT << ODBCXX_STRING_CONST("Deleted row ") << i << endl;
    }
    i++;
  }

  rs=std::auto_ptr<ResultSet>(stmt->executeQuery
    (ODBCXX_STRING_CONST("select id,name,ts from ")+tableName));
  i=1;
  while(rs->next()) {
    basic_ostringstream<ODBCXX_CHAR_TYPE> ns;
    ns << ODBCXX_STRING_CONST("This IS row number ") << i;
    ODBCXX_STRING name(ns.str());

    ASSERT(rs->getString(ODBCXX_STRING_CONST("name"))==name);
    ASSERT(rs->getInt(1)==i);

    i+=2;
  }

  ASSERT(i==TABLE_ROWS+1);

  con->commit();
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

    int numTests=3;
    int failedTests=0;

    con->setAutoCommit(false);
    dropStuff(con.get());
    createStuff(con.get());
    try {
      testProc(con.get());
    } catch(SQLException& e) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Procedure test FAILED: ")
                  << e.getMessage() << endl;
      failedTests++;
    }

    try {
      testFunc(con.get());
    } catch(SQLException& e) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Function test FAILED: ")
                  << e.getMessage() << endl;
      failedTests++;
    }

    try {
      testTable(con.get());
    } catch(SQLException& e) {
      ODBCXX_COUT << ODBCXX_STRING_CONST("Table test FAILED: ")
                  << e.getMessage() << endl;
      failedTests++;
    }

    dropStuff(con.get());

    if(failedTests>0) {
      ODBCXX_COUT << failedTests << ODBCXX_STRING_CONST(" of ") << numTests
                  << ODBCXX_STRING_CONST(" tests failed.") << endl;
    }
  } catch(exception& e) {
    cout << "Whoops: " << e.what() << endl;
    return 1;
  }

  if(assertionsFailed>0) {
    ODBCXX_COUT << assertionsFailed << ODBCXX_STRING_CONST(" assertions failed.") << endl;
    return 2;
  }
  return 0;
}

#else
int main() { return 0; }
#endif
