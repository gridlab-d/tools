#ifndef __ISQLXX_H
#define __ISQLXX_H

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/databasemetadata.h>

#include "dtconv.h"

#include <map>
#include <cstring>
#include <stdexcept>

using namespace odbc;
using namespace std;

#if 0
template <class T>
class Deleter {
private:
  T* val_;
  Deleter(const Deleter<T>&);
  Deleter<T>& operator=(const Deleter<T>&);

public:
  Deleter(T* t) : val_(t) {}
  ~Deleter() { delete val_; }
};
#endif


class Isql {
public:

  class CommandException : public exception {
  public:
    CommandException(const string& str) : msg_(str) {}
    virtual ~CommandException() throw() {}
    
    virtual const char* what() const throw() {
      return msg_.c_str();
    }

  private:
    string msg_;
  };
  

  Isql(Connection* con);
  virtual ~Isql();

  // returns false if EOF
  // otherwise, sets s to the string read
  bool readInput(string& s);

  void run();

  // execute pstmt and dump the results (if any)
  void execute(PreparedStatement* pstmt);

  // just dump a result set
  void printResultSet(ResultSet* rs);
  void showAffected();



  size_t getTermWidth() const {
    return termWidth_;
  }

  void setTermWidth(size_t w) {
    termWidth_=w;
  }

  enum StatementType {
    STATEMENT_COMMAND,
    STATEMENT_SQL,
    STATEMENT_NONE
  };

  

  // Tries to extract a statement from the buffer
  // into s and returns it's type. If there is anything
  // after the statement, it's left there.
  StatementType extractStatement(string& s);

  void executeCommand(const string& cmd);


  // commands
  void setCmd(const vector<string>& args);
  void showCmd(const vector<string>& args);
  void showTypesCmd();

  void commitCmd(const vector<string>& args);
  void rollbackCmd(const vector<string>& args);

  void describeCmd(const vector<string>& args);


private:

  void splitIdentifier(const string& id,
		       string& cat,string& sch, string& name);
  string makeIdentifier(const string& cat,
			const string& schema,
			const string& name);

  Connection* con_;
  size_t termWidth_;
  string buffer_;

  struct ltcasestr {
    bool operator()(const string& s1, const string& s2) {
      return strcasecmp(s1.c_str(),s2.c_str())<0;
    }
  };

  typedef void (Isql::*Command)(const vector<string>& args);

  typedef map<string,Command,ltcasestr> CmdMap;
  CmdMap commands_;

  bool supportsSchemas_;
  bool supportsCatalogs_;

  // variable values
  int maxRows_;
};


#endif
