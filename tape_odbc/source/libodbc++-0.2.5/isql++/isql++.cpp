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

#include "isql++.h"

#include <cstdlib>
#include <cstdio>
#include <iostream>

extern "C" {
#if defined(ODBCXX_DISABLE_READLINE_HACK)
#include <readline/readline.h>
#include <readline/history.h>
#else

  /* readline.h doesn't contain proper function prototypes, which makes
     newer gcc versions (>=2.95) barf with certain flags. This could
     help the situation.
  */
  extern char* rl_readline_name;
  
  typedef char** (*CPPFunction)(char*,char*);
  
  extern CPPFunction rl_completion_entry_function;
  extern int rl_initialize(void);
  extern char* readline(const char*);
  extern void add_history(const char*);

#ifdef SPACE
#undef SPACE
#endif
#define SPACE ' '

#endif
}

using namespace odbc;
using namespace std;

const char* SQLPROMPT1="SQL> ";
const char* SQLPROMPT2="  +> ";
const char* BLOB_FIELD="<BLOB>";
const char* NULL_FIELD="<NULL>";
const char* INNER_SEPARATOR=" ";
const char* OUTER_SEPARATOR=" ";
const char SPACE_CHAR=' ';
const char LINE_CHAR='=';
const char END_OF_STATEMENT=';';
const char* WS=" \r\n\t";

const int LONGVARCHAR_WIDTH=20;
const int MIN_COL_WIDTH_ON_SCREEN=4;

// this effectively disables filename completion
// with readline
static char **noCompletion (char *,char*)
{
  // no completion of filenames
  return (char**)NULL;
}

static const char* getTypeName(int sqlType) {
  static struct {
    int id;
    const char* name;
  } sqlTypes[] = {
    { Types::BIGINT,		"BIGINT" },
    { Types::BINARY,		"BINARY" },
    { Types::BIT,		"BIT" },
    { Types::CHAR,		"CHAR" },
    { Types::DATE,		"DATE" },
    { Types::DECIMAL,		"DECIMAL" },
    { Types::DOUBLE, 		"DOUBLE" },
    { Types::FLOAT,		"FLOAT" },
    { Types::INTEGER,		"INTEGER" },
    { Types::LONGVARBINARY, 	"LONGVARBINARY" },
    { Types::LONGVARCHAR, 	"LONGVARCHAR" },
    { Types::NUMERIC, 	"NUMERIC" },
    { Types::REAL,		"REAL" },
    { Types::SMALLINT,	"SMALLINT" },
    { Types::TIME,		"TIME" },
    { Types::TIMESTAMP,	"TIMESTAMP" },
    { Types::TINYINT,		"TINYINT" },
    { Types::VARBINARY, 	"VARBINARY" },
    { Types::VARCHAR, 	"VARCHAR" },
    {0,			NULL }
  };
  
  for(unsigned int i=0; sqlTypes[i].name!=NULL; i++) {
    if(sqlTypes[i].id==sqlType) {
      return sqlTypes[i].name;
    }
  }
  
  return "UNKNOWN";
}

// split string on any of schars
inline vector<string> 
splitString(const string& str, const char* schars)
{
  vector<string> res;
  
  if(str.length()==0) {
    return res;
  }

  if(strlen(schars)==0) {
    res.push_back(str);
    return res;
  }
  
  string::size_type e=0;

  string::size_type s=str.find_first_not_of(schars);
  
  while(s!=string::npos) {
    e=str.find_first_of(schars,s);
    if(e==string::npos) {
      res.push_back(str.substr(s));
    } else {
      res.push_back(str.substr(s,e-s));
    }
    s=str.find_first_not_of(schars,e);
  }

  for(vector<string>::iterator i=res.begin();
      i!=res.end(); i++) {
  }

  return res;
}

// lowercase a string
inline string toLowerCase(const string& str) {
  string ret;
  
  if(str.length()>0) {
    ret.resize(str.length());
    for(int i=0; i<str.length(); i++) {
      ret[i]=tolower(str[i]);
    }
  }
  return ret;
}

inline string toUpperCase(const string& str) {
  string ret;
  
  if(str.length()>0) {
      ret.resize(str.length());
      for(int i=0; i<str.length(); i++) {
	ret[i]=toupper(str[i]);
      }
  }
  return ret;
}


// split a fully qualified sql table or procedure
// identifier into catalog,schema and name
// returns a vector of all three of them
void Isql::splitIdentifier(const string& id,
			   string& catalog,
			   string& schema,
			   string& name)
{
  vector<string> ret;

  DatabaseMetaData* md=con_->getMetaData();

  string str=id; //local copy, we are going to modify it
  if(supportsCatalogs_) {
    string catSep=md->getCatalogSeparator();
    string::size_type catStart, catEnd;
    if(md->isCatalogAtStart()) {
      catStart=0;
      catEnd=str.find(catSep);
      if(catEnd!=string::npos) {
	catalog=str.substr(0,catEnd);
	str=str.substr(catEnd+catSep.length());
      }
    } else {
      catStart=str.rfind(catSep);
      if(catStart!=string::npos) {
	catalog=str.substr(catStart+1);
	str=str.substr(0,catStart);
      }
    }
  } else {
    catalog="";
  }

  // check for schemas
  if(supportsSchemas_) {
    string schemaSep=".";
    
    string::size_type schemaEnd=str.find(schemaSep);
    if(schemaEnd!=string::npos) {
      schema=str.substr(0,schemaEnd);
      str=str.substr(schemaEnd+schemaSep.length());
    }
  } else {
    schema="";
  }

  name=str;

  //now, check if we are to perform some case-transforms on this
  //since some drivers can't refer to table test using TEST
  //when metadata information is requested
  
  if(md->storesLowerCaseIdentifiers()) {
    name=toLowerCase(name);
    schema=toLowerCase(schema);
    catalog=toLowerCase(catalog);
  } else if(md->storesUpperCaseIdentifiers()) {
    name=toUpperCase(name);
    schema=toUpperCase(schema);
    catalog=toUpperCase(catalog);
  }

  //otherwise, we don't touch them
}

string Isql::makeIdentifier(const string& cat,
			    const string& schema,
			    const string& name)
{
  string id=name;
  DatabaseMetaData* md=con_->getMetaData();

  if(supportsSchemas_ && schema.length()>0) {
    string schemaSep=".";
    id=schema+schemaSep+id;
  }

  if(supportsCatalogs_ && cat.length()>0) {
    string catSep=md->getCatalogSeparator();
    if(md->isCatalogAtStart()) {
      id=cat+catSep+id;
    } else {
      id+=(catSep+cat);
    }
  }
  return id;
}




Isql::Isql(Connection* con)
  :con_(con),termWidth_(80),
   maxRows_(0)
{
  rl_readline_name="isqlxx";
  rl_completion_entry_function=(CPPFunction)noCompletion;
  rl_initialize();

  commands_["set"]=&Isql::setCmd;
  commands_["show"]=&Isql::showCmd;
  commands_["commit"]=&Isql::commitCmd;
  commands_["rollback"]=&Isql::rollbackCmd;
  commands_["describe"]=&Isql::describeCmd;

  // a cheap way to check if this supports schemas
  try {
    ResultSet* rs=con_->getMetaData()->getSchemas();
    Deleter<ResultSet> _rs(rs);
    supportsSchemas_=rs->next();
  } catch(SQLException& e) {
    supportsSchemas_=true;
  }

  // another cheap trick
  try {
    ResultSet* rs=con_->getMetaData()->getCatalogs();
    Deleter<ResultSet> _rs(rs);
    supportsCatalogs_=rs->next();
  } catch(SQLException& e) {
    supportsCatalogs_=true;
  }

}

Isql::~Isql()
{
  //safety
  if(con_->getMetaData()->supportsTransactions()) {
    con_->rollback();
  }
}

bool Isql::readInput(string& out)
{
  const char* prompt=(buffer_.length()>0?SQLPROMPT2:SQLPROMPT1);
  char* s=readline(prompt);
  
  if(s!=NULL) {
    
    if(s[0]!=0) {
      add_history(s);
      out=s;
    } else {
      out = "";
    }
    //free it (it is malloced)
    free(s);

    return true;

  } else {
    return false;
  }
}



void Isql::run()
{
  string input;
  
  while(this->readInput(input)) {
    buffer_=buffer_+(buffer_.length()>0?"\n"+input:input);
    string s;
    StatementType st=this->extractStatement(s);
    
    switch(st) {
    case STATEMENT_SQL:
      try {
	if(s.length()>1 && 
	   s[0]=='"' && s[s.length()-1]=='"') {
	  //the whole thing is quoted, strip the quotes
	  s=s.substr(1,s.length()-2);
	}
	PreparedStatement* pstmt=con_->prepareStatement(s);
	Deleter<PreparedStatement> _pstmt(pstmt);

	//apply maxrows if needed
	if(maxRows_>0) {
	  pstmt->setMaxRows(maxRows_);
	}

	this->execute(pstmt);
      } catch(SQLException& e) {
	cout << e.getMessage() << endl;
      }
      break;

    case STATEMENT_COMMAND:
      try {
	this->executeCommand(s);
      } catch(exception& e) {
	cout << e.what() << endl;
      }
      break;
    case STATEMENT_NONE:
      break;
    }
  }
}

void Isql::execute(PreparedStatement* pstmt)
{
  if(pstmt->execute()) {
    ResultSet* rs=pstmt->getResultSet();
    Deleter<ResultSet> _rs(rs);
    this->printResultSet(rs);
  }

  try {
    int ar=pstmt->getUpdateCount();
    cout << OUTER_SEPARATOR << ar << " rows affected" << flush;
  } catch(SQLException& e) {} //ignore it
  cout << endl;
}

void Isql::printResultSet(ResultSet* rs)
{
  int cnt=0;
  
  ResultSetMetaData* md=rs->getMetaData();
  
  int totalWidth=0, shrinkableWidth=0;
  vector<int> widths;
  for(int i=1; i<=md->getColumnCount(); i++) {
    int l;
    switch(md->getColumnType(i)) {
    case Types::LONGVARCHAR:
      l=LONGVARCHAR_WIDTH;
      break;
    case Types::LONGVARBINARY:
      l=strlen(BLOB_FIELD);
      break;
    default:
      l=md->getColumnDisplaySize(i);
      break;
    }
    widths.push_back(l);
    totalWidth+=l;
    if ( i > 1 ) totalWidth += strlen(INNER_SEPARATOR);
  }
  totalWidth += 2 * strlen(OUTER_SEPARATOR);
  
  if(totalWidth>(int)termWidth_) {
    //we'll need to shrink some column widths
    //we only do that on char and varchar columns
    
    for(int i=1; i<=md->getColumnCount(); i++) {
      switch(md->getColumnType(i)) {
      case Types::CHAR:
      case Types::VARCHAR:
        shrinkableWidth+=md->getColumnDisplaySize(i);
        break;
      default:
        break;
      }
    }
    
    if(shrinkableWidth>0) {
      //ok, now try shrinking
      float shrinkFactor = (1.0 - ((float)(totalWidth - (int)termWidth_)) / (float)shrinkableWidth);

      // if shrinkFactor is negative will use a minimal width
      int w;
      for(int i=0; i < (int)widths.size(); i++) {
        switch( md->getColumnType(i+1) ) {
        case Types::CHAR:
        case Types::VARCHAR:
	      // cout << "Column " << md->getColumnName(i+1)
          //  << "; shrinked from " << widths[i] << flush;
	      w = (int)( ((float)widths[i]) * shrinkFactor);
	      widths[i] = ( w > MIN_COL_WIDTH_ON_SCREEN ) ? w : MIN_COL_WIDTH_ON_SCREEN;
          // cout  << " to " << widths[i] << endl;
	      break;
        default:
          break;
        }
      }
    }
  }  
  
  //now we have all widths, print column names
  cout << OUTER_SEPARATOR << flush;
  for(int i=1; i<=md->getColumnCount(); i++) {
    int w=widths[i-1];
    string colName=md->getColumnName(i);
    //fix the width
    colName.resize(w,SPACE);
    if(i>1) {
      cout << INNER_SEPARATOR << flush;
    }
    cout << colName << flush;
  }
  cout << OUTER_SEPARATOR << endl;
  
  //print a line
  cout << OUTER_SEPARATOR << flush;
  for(int i=1; i<=md->getColumnCount(); i++) {
    int w=widths[i-1];
    string s;
    s.resize(w,LINE_CHAR);
    if(i>1) {
      cout << INNER_SEPARATOR << flush;
    }
    cout << s << flush;
  }
  cout << OUTER_SEPARATOR << endl;
  
  
  //and finally, actually print the results
  while(rs->next()) {
    cnt++;
    cout << OUTER_SEPARATOR << flush;
    for(int i=1; i<=md->getColumnCount(); i++) {
      int w=widths[i-1];
      string val=rs->getString(i);
      if(rs->wasNull()) {
	val=NULL_FIELD;
      }
      val.resize(w,SPACE);
      if(i>1) {
	cout << INNER_SEPARATOR << flush;
      }
      cout << val << flush;
    }
    cout << OUTER_SEPARATOR << endl;
  }
  cout << endl
       << OUTER_SEPARATOR << cnt << " rows fetched." << endl;
} 



Isql::StatementType
Isql::extractStatement(string& out)
{
  // clear out
  out="";

  if(buffer_.length()==0) {
    return STATEMENT_NONE;
  }

  //first, find the first non-whitespace char
  
  string::size_type first=
    buffer_.find_first_not_of(WS);

  if(first==string::npos) {
    return STATEMENT_NONE;
  }

  StatementType st=STATEMENT_NONE;


  string::size_type firstEnd=
    buffer_.find_first_of(WS,first);
  
  if(firstEnd!=string::npos ||
     buffer_.length()-1>first) {
    
    // we need to check whether we brought an ; with us
    
    string::size_type eosp=buffer_.find_first_of
      (END_OF_STATEMENT,first);
    
    if(eosp!=string::npos &&
       (firstEnd==string::npos || firstEnd>eosp)) {
      firstEnd=eosp;
    }
    
    string cmd=buffer_.substr(first,firstEnd-first);
    
    CmdMap::iterator i;

    if((i=commands_.find(cmd))!=commands_.end()) {
      st = STATEMENT_COMMAND;
    }
  }

  if(st==STATEMENT_NONE) {
    // our only choice now
    st=STATEMENT_SQL;
  }

  //ok, we know what we have in there.
  //now, let's search for a terminator

  const char* str=buffer_.c_str();

  int inQ=0; //this is either '\'', '"' or 0 while iterating
  int i=0;

  while(str[i]!=0) {
    char c=str[i];
    switch(c) {
    case '\'':
      if(inQ==0) {
	inQ='\'';
      } else if(inQ=='\'') {
	inQ=0;
      }
      break;

    case '"':
      if(inQ==0) {
	inQ='"';
      } else if(inQ=='"') {
	inQ=0;
      }
      break;

    case END_OF_STATEMENT:
      if(inQ==0) {
	out=buffer_.substr(0,(string::size_type)i);
	if(str[i+1]!=0) {
	  buffer_=buffer_.substr((string::size_type)i+1);
	} else {
	  buffer_="";
	}
	return st;
      }
      break;
      
    default:
      break;
    };
    i++;
  }

  return STATEMENT_NONE;
}


void Isql::executeCommand(const string& cmd)
{
  // we need to split up cmd into tokens
  
  vector<string> tokens=splitString(cmd,WS);
  if(tokens.size()==0) {
    cout << "Whoops: 0 tokens for executeCommand() " << endl;
    return;
  }

  vector<string>::iterator i=tokens.begin();
  string name=*i;
  vector<string> args;
  while((++i)!=tokens.end()) {
    args.push_back(*i);
  }

  CmdMap::iterator cmdi=commands_.find(name);
  if(cmdi!=commands_.end()) {
    Command cmd=(*cmdi).second;
    (this->*cmd)(args);
  }
}





void Isql::setCmd(const vector<string>& args)
{
  if(args.size()!=2) {
    throw CommandException
      ("invalid number of arguments to 'set'");
  }

  string name=toLowerCase(args[0]);
  string value=args[1];

  if(name=="maxrows") {
    int i=stringToInt(value);
    if(i<0) {
      throw CommandException
	("maxrows must be >= 0");
    }
    maxRows_=i;
  } else if(name=="catalog") {
    con_->setCatalog(value);
  } else if(name=="autocommit") {
    string v=toLowerCase(value);
    bool ac=(v=="on"?true:false);
    con_->setAutoCommit(ac);
  } else if(name=="trace") {
    string v=toLowerCase(value);
    bool t=(v=="on"?true:false);
    con_->setTrace(t);
  } else if(name=="tracefile") {
    con_->setTraceFile(value);
  } else {
    throw CommandException
      ("set: unknown variable '"+name+"'");
  }
}

void Isql::showCmd(const vector<string>& args)
{
  if(args.size()!=1) {
    throw CommandException
      ("invalid number of arguments to 'show'");
  }

  string name=toLowerCase(args[0]);
  
  string value;

  if(name=="maxrows") {
    value=intToString(maxRows_);
  } else if(name=="catalog") {
    value=con_->getCatalog();
  } else if(name=="autocommit") {
    value=(con_->getAutoCommit()?"on":"off");
  } else if(name=="trace") {
    value=(con_->getTrace()?"on":"off");
  } else if(name=="tracefile") {
    value=(con_->getTraceFile());
  } else if(name=="types") {
    this->showTypesCmd();
    return;
  } else {
    throw CommandException
      ("show: unknown variable '"+name+"'");
  }
  cout << name << " is " << value << endl;
}


void Isql::showTypesCmd()
{
  DatabaseMetaData* md=con_->getMetaData();
  try {
    ResultSet* rs=md->getTypeInfo();
    Deleter<ResultSet> _rs(rs);

    if(rs->next()) {
      int nameLen=25;
      int odbcTypeLen=13;
      int maxLenLen=15;
      int paramsLen=20;
      int nullableLen=4;
      int searchableLen=12;
      
      string nameTitle="Name";
      string odbcTypeTitle="ODBC Type";
      string maxLenTitle="Max size";
      string paramsTitle="Create with";
      string nullableTitle="Null";
      string searchableTitle="Searchable";

      nameTitle.resize(nameLen,SPACE_CHAR);
      odbcTypeTitle.resize(odbcTypeLen,SPACE_CHAR);
      maxLenTitle.resize(maxLenLen,SPACE_CHAR);
      paramsTitle.resize(paramsLen,SPACE_CHAR);
      nullableTitle.resize(nullableLen,SPACE_CHAR);
      searchableTitle.resize(searchableLen,SPACE_CHAR);

      cout << OUTER_SEPARATOR << nameTitle
	   << INNER_SEPARATOR << odbcTypeTitle
	   << INNER_SEPARATOR << maxLenTitle 
	   << INNER_SEPARATOR << paramsTitle
	   << INNER_SEPARATOR << nullableTitle
	   << INNER_SEPARATOR << searchableTitle
	   << OUTER_SEPARATOR << endl;

      string t;
      t.resize(nameLen,LINE_CHAR);
      cout << OUTER_SEPARATOR << t;
      t.resize(odbcTypeLen,LINE_CHAR);
      cout << INNER_SEPARATOR << t;
      t.resize(maxLenLen,LINE_CHAR);
      cout << INNER_SEPARATOR << t;
      t.resize(paramsLen,LINE_CHAR);
      cout << INNER_SEPARATOR << t;
      t.resize(nullableLen,LINE_CHAR);
      cout << INNER_SEPARATOR << t;
      t.resize(searchableLen,LINE_CHAR);
      cout << INNER_SEPARATOR << t
	   << OUTER_SEPARATOR << endl;
      do {
	string name=rs->getString(1);
	string odbcType=getTypeName(rs->getShort(2));
	string params=rs->getString(6);

	if(rs->wasNull()) {
	  params="";
	}

	int ml=rs->getInt(3);
	string maxLen;
	if(!rs->wasNull()) {
	  maxLen=intToString(ml);
	}

	string nullable;
	switch(rs->getShort(7)) {
	case DatabaseMetaData::typeNoNulls:
	  nullable="No"; break;
	case DatabaseMetaData::typeNullable:
	  nullable="Yes"; break;
	case DatabaseMetaData::typeNullableUnknown:
	default:
	  nullable="?"; break;
	}

	string searchable;
	switch(rs->getShort(9)) {
	case DatabaseMetaData::typePredChar:
	  searchable="LIKE only"; break;
	case DatabaseMetaData::typePredBasic:
	  searchable="except LIKE"; break;
	case DatabaseMetaData::typeSearchable:
	  searchable="Yes"; break;
	case DatabaseMetaData::typePredNone:
	default:
	  searchable="No"; break;
	}

	name.resize(nameLen,SPACE_CHAR);
	odbcType.resize(odbcTypeLen,SPACE_CHAR);
	maxLen.resize(maxLenLen,SPACE_CHAR);
	params.resize(paramsLen,SPACE_CHAR);
	nullable.resize(nullableLen,SPACE_CHAR);
	searchable.resize(searchableLen,SPACE_CHAR);

	cout << OUTER_SEPARATOR << name
	     << INNER_SEPARATOR << odbcType
	     << INNER_SEPARATOR << maxLen 
	     << INNER_SEPARATOR << params
	     << INNER_SEPARATOR << nullable
	     << INNER_SEPARATOR << searchable
	     << OUTER_SEPARATOR << endl;

      } while(rs->next());
    }

  } catch(SQLException& e) {
    cout << e.getMessage() << endl;
  }
}


void Isql::commitCmd(const vector<string>& args)
{
  //we don't care about arguments, there are like 20 different
  //commit syntaxes
  try {
    con_->commit();
    cout << "Transaction committed." << endl;
  } catch(SQLException& e) {
    cout << e.getMessage() << endl;
  }
}

void Isql::rollbackCmd(const vector<string>& args)
{
  //same here
  try {
    con_->rollback();
    cout << "Transaction rolled back." << endl;
  } catch(SQLException& e) {
    cout << e.getMessage() << endl;
  }
}


void Isql::describeCmd(const vector<string>& args)
{
  if(args.size()!=1) {
    throw CommandException
      ("Invalid number of arguments to 'describe'");
  }
  
  string arg=args[0];

  DatabaseMetaData* md=con_->getMetaData();

  string catalog;
  string schema;
  string name;

  this->splitIdentifier(arg,catalog,schema,name);

  vector<string> tableTypes;
  
  // fetch a list of all table types
  {
    ResultSet* rs=md->getTableTypes();
    Deleter<ResultSet> _rs(rs);
    while(rs->next()) {
      tableTypes.push_back(rs->getString("TABLE_TYPE"));
    }
  }
  
  // first, we check for a table or view
  bool wasTable;
  {

    ResultSet* tableRs=md->getTables(catalog,schema,name,tableTypes);
    Deleter<ResultSet> _tableRs(tableRs);
    
    if((wasTable=tableRs->next())) {
      do {
	cout << this->makeIdentifier
	  (tableRs->getString(1),tableRs->getString(2),tableRs->getString(3));
	
	cout << " (" << tableRs->getString(4) << ")" << endl;

	string desc=tableRs->getString(5);
	bool descIsNull=tableRs->wasNull();

	int nameLength=30;
	int nullabilityLength=10;
	int typeLength=25;

	string nameTitle="Column name";
	string nullabilityTitle="Nullable";
	string typeTitle="Data type";
	
	nameTitle.resize(nameLength,SPACE_CHAR);
	typeTitle.resize(typeLength,SPACE_CHAR);
	nullabilityTitle.resize(nullabilityLength,SPACE_CHAR);
	
	cout << OUTER_SEPARATOR << nameTitle
	     << INNER_SEPARATOR << typeTitle
	     << INNER_SEPARATOR << nullabilityTitle
	     << OUTER_SEPARATOR << endl;

	string t;
	cout << OUTER_SEPARATOR;
	t.resize(nameLength,LINE_CHAR);
	cout << t << INNER_SEPARATOR;
	t.resize(typeLength,LINE_CHAR);
	cout << t << INNER_SEPARATOR;
	t.resize(nullabilityLength,LINE_CHAR);
	cout << t << OUTER_SEPARATOR << endl;
	
	//now, fetch the columns
	{
	  ResultSet* rs=md->getColumns
	    (catalog,schema,tableRs->getString(3),"%");
	  Deleter<ResultSet> _rs(rs);
	  
	  while(rs->next()) {
	    cout << OUTER_SEPARATOR;

	    string colName=rs->getString(4);
	    colName.resize(nameLength,SPACE_CHAR);

	    string typeName=rs->getString(6);
	    typeName.resize(typeLength,SPACE_CHAR);
	    
	    string nullInfo;
	    switch(rs->getInt(11)) {
	    case DatabaseMetaData::columnNoNulls:
	      nullInfo="No";
	      break;
	    case DatabaseMetaData::columnNullable:
	      nullInfo="Yes";
	      break;
	    default:
	      nullInfo="Unknown";
	      break;
	    }

	    nullInfo.resize(nullabilityLength);
	    
	    cout << colName << INNER_SEPARATOR
		 << typeName << INNER_SEPARATOR
		 << nullInfo << OUTER_SEPARATOR
		 << endl;
	  }
	}

	cout << endl;

	try {
	  ResultSet* rs=md->getPrimaryKeys(tableRs->getString(1),
					   tableRs->getString(2),
					   tableRs->getString(3));
	  Deleter<ResultSet> _rs(rs);
	  string colNames;
	  string keyName;
	  bool keyNameIsNull=true;
	  if(rs->next()) {
	    try {
	      keyName=rs->getString(6);
	      keyNameIsNull=rs->wasNull();
	    } catch(SQLException&) {} //temporary workaround for OL driver

	    do {
	      if(colNames.length()>0) {
		colNames+=",";
	      }
	      colNames+=rs->getString(4);
	    } while(rs->next());
	  }

	  if(colNames.length()>0) {
	    cout << OUTER_SEPARATOR << "PRIMARY KEY "
		 << "(" << colNames << ")" << endl;
	      
	  }
	} catch(SQLException& e) {
	}

	//fetch the foreign keys
	try {
	  ResultSet* rs=md->getImportedKeys(tableRs->getString(1),
					    tableRs->getString(2),
					    tableRs->getString(3));
	  
	  Deleter<ResultSet> _rs(rs);
	  
	  bool goon=rs->next();
	  // here, we trust that the result set is ordered
	  // by key_seq
	  while(goon) {
	    string pkTable=this->makeIdentifier
	      (rs->getString(1),rs->getString(2),rs->getString(3));

	    string pkCols;
	    string fkCols;
	    short ks=rs->getShort(9);
	    
	    pkCols+=rs->getString(4);
	    fkCols+=rs->getString(8);
	    
	    while(goon=rs->next()) {
	      if(rs->getShort(9)==1) {
		//this is part of another key
		break;
	      }
	      
	      pkCols+=",";
	      fkCols+=",";
	      
	      pkCols+=rs->getString(4);
	      fkCols+=rs->getString(8);
	    }
	    
	    cout << OUTER_SEPARATOR << "FOREIGN KEY ("
		 << fkCols << ") REFERENCES " 
		 << pkTable << " (" << pkCols << ")"
		 << OUTER_SEPARATOR << endl;
	  }
	} catch(SQLException& e) {
	  // ignore, probably not supported
	}

	try {
	  ResultSet* rs=md->getIndexInfo(tableRs->getString(1),
					 tableRs->getString(2),
					 tableRs->getString(3),
					 false,false);
	  Deleter<ResultSet> _rs(rs);
	  bool goon=rs->next();

	  while(goon) {
	    //ignore statistics
	    if(rs->getShort(7)==DatabaseMetaData::tableIndexStatistic) {
	      goon=rs->next();
	      break;
	    }
	    
	    string idxName=rs->getString(6);
	    string cols=rs->getString(9);
	    string order=(rs->getString(10)=="A"?"ASC":"DESC");
	    string unique=(rs->getBoolean(4)?"":"UNIQUE ");
	    
	    while(goon=rs->next()) {
	      if(rs->getShort(8)==1) {
		// part of next index
		break;
	      }
	      cols+=","+rs->getString(9);
	    }

	    cout << OUTER_SEPARATOR << unique << order
		 << " INDEX " << idxName << " ON (" << cols << ")" << endl;
	  }
	  

	} catch(SQLException&) {}

	try {
	  ResultSet* rs=md->getBestRowIdentifier(tableRs->getString(1),
						 tableRs->getString(2),
						 tableRs->getString(3),
						 DatabaseMetaData::bestRowTemporary,
						 true);
	  Deleter<ResultSet> _rs(rs);

	  if(rs->next()) {

	    cout << endl;

	    int nameLength=30;
	    int pseudoLength=10;
	    int scopeLength=30;

	    string nameTitle="Column name";
	    string pseudoTitle="Pseudo";
	    string scopeTitle="Scope";

	    cout << OUTER_SEPARATOR << "Best row identifiers" << endl;
	
	    nameTitle.resize(nameLength,SPACE_CHAR);
	    pseudoTitle.resize(pseudoLength,SPACE_CHAR);
	    scopeTitle.resize(scopeLength,SPACE_CHAR);
	
	    cout << OUTER_SEPARATOR << nameTitle
		 << INNER_SEPARATOR << pseudoTitle
		 << INNER_SEPARATOR << scopeTitle
		 << OUTER_SEPARATOR << endl;
	    
	    string t;
	    cout << OUTER_SEPARATOR;
	    t.resize(nameLength,LINE_CHAR);
	    cout << t << INNER_SEPARATOR;
	    t.resize(pseudoLength,LINE_CHAR);
	    cout << t << INNER_SEPARATOR;
	    t.resize(scopeLength,LINE_CHAR);
	    cout << t << OUTER_SEPARATOR << endl;

	    do {
	      string colName=rs->getString(2);
	      string pseudo;
	      string scope;

	      switch(rs->getInt(8)) {
	      case DatabaseMetaData::bestRowPseudo:
		pseudo="Yes";
		break;
	      case DatabaseMetaData::bestRowNotPseudo:
		pseudo="No";
		break;
	      case DatabaseMetaData::bestRowUnknown:
		pseudo="Unknown";
		break;
	      default:
		pseudo="?";
		break;
	      }

	      switch(rs->getInt(1)) {
	      case DatabaseMetaData::bestRowTemporary:
		scope="While positioned on row";
		break;
	      case DatabaseMetaData::bestRowTransaction:
		scope="Current transaction";
		break;
	      case DatabaseMetaData::bestRowSession:
		scope="Session (across transactions)";
		break;

	      default:
		scope="?";
		break;
	      }

	      colName.resize(nameLength,SPACE_CHAR);
	      pseudo.resize(pseudoLength,SPACE_CHAR);
	      scope.resize(scopeLength,SPACE_CHAR);

	      cout << OUTER_SEPARATOR << colName
		   << INNER_SEPARATOR << pseudo
		   << INNER_SEPARATOR << scope
		   << OUTER_SEPARATOR << endl;

	    } while(rs->next());
	  } 
	} catch(SQLException& e) {
	  // ignore
	}

	cout << endl;
	
	if(!descIsNull && desc.length()>0) {
	  cout << OUTER_SEPARATOR << "Description" << endl;
	  cout << OUTER_SEPARATOR << "===========" << endl;
	  cout << OUTER_SEPARATOR << desc << endl;
	  cout << endl;
	}
      } while(tableRs->next());
    }
  }

  bool wasProcedure=false;

  if(!wasTable) {
    // this might be a procedure
    try {
      ResultSet* procRs=md->getProcedures(catalog,schema,name);
      Deleter<ResultSet> _procRs(procRs);
      
      if(wasProcedure=procRs->next()) {
	do {
	  cout << this->makeIdentifier
	    (procRs->getString(1),procRs->getString(2),procRs->getString(3))
	       << " (PROCEDURE)" << endl;

	  ResultSet* colRs=md->getProcedureColumns(procRs->getString(1),
						   procRs->getString(2),
						   procRs->getString(3),
						   "%");
	  Deleter<ResultSet> _colRs(colRs);

	  if(colRs->next()) {
	    int nameLen=30;
	    int dataTypeLen=15;
	    int nullableLen=8;
	    int typeLen=10;

	    string nameTitle="Column";
	    string dataTypeTitle="Data type";
	    string nullableTitle="Nullable";
	    string typeTitle="Type";

	    nameTitle.resize(nameLen,SPACE_CHAR);
	    dataTypeTitle.resize(dataTypeLen,SPACE_CHAR);
	    nullableTitle.resize(nullableLen);
	    typeTitle.resize(typeLen);
	    
	    cout << OUTER_SEPARATOR << nameTitle
		 << INNER_SEPARATOR << dataTypeTitle
		 << INNER_SEPARATOR << nullableTitle
		 << INNER_SEPARATOR << typeTitle
		 << OUTER_SEPARATOR << endl;
	    string t;
	    t.resize(nameLen,LINE_CHAR);
	    cout << OUTER_SEPARATOR << t;
	    t.resize(dataTypeLen,LINE_CHAR);
	    cout << INNER_SEPARATOR << t;
	    t.resize(nullableLen,LINE_CHAR);
	    cout << INNER_SEPARATOR << t;
	    t.resize(typeLen,LINE_CHAR);
	    cout << t << OUTER_SEPARATOR << endl;
	    do {
	      string name=colRs->getString(4);
	      string dataType=colRs->getString(7);

	      string nullable;
	      switch(colRs->getShort(12)) {
	      case DatabaseMetaData::procedureNoNulls:
		nullable="No";
		break;
	      case DatabaseMetaData::procedureNullable:
		nullable="Yes";
		break;
	      case DatabaseMetaData::procedureNullableUnknown:
	      default:
		nullable="?";
		break;
	      }
	      
	      string type;
	      switch(colRs->getShort(5)) {
	      case DatabaseMetaData::procedureColumnIn:
		type="IN"; break;
	      case DatabaseMetaData::procedureColumnOut:
		type="OUT"; break;
	      case DatabaseMetaData::procedureColumnInOut:
		type="IN OUT"; break;
	      case DatabaseMetaData::procedureColumnReturn:
		type="RETURN"; break;
	      case DatabaseMetaData::procedureColumnResult:
		type="RESULT"; break;
	      default:
		type="?";
	      }

	      name.resize(nameLen,SPACE_CHAR);
	      dataType.resize(dataTypeLen,SPACE_CHAR);
	      nullable.resize(nullableLen,SPACE_CHAR);
	      type.resize(typeLen,SPACE_CHAR);

	      cout << OUTER_SEPARATOR << name
		   << INNER_SEPARATOR << dataType
		   << INNER_SEPARATOR << nullable
		   << INNER_SEPARATOR << type
		   << OUTER_SEPARATOR << endl;

	    } while(colRs->next());
	  }
	  
	} while(procRs->next());
      }
      
    } catch(SQLException& e) {
      //apparently no procedures
    }
  }

  if(!(wasProcedure || wasTable)) {
    cout << "Could not find any table or procedure named " << arg << endl;
  }
}




int main(int argc, char** argv)
{
  if(argc!=2 && argc!=4) {
    cerr << "Usage: " << argv[0] << " connect-string" << endl
	 << "or     " << argv[0] << " dsn username password" << endl;
    return 0;
  }
  try {
    Connection* con;
    if(argc==2) {
      con=DriverManager::getConnection(argv[1]);
    } else {
      con=DriverManager::getConnection(argv[1],argv[2],argv[3]);
    }

    Deleter<Connection> _con(con);
    DatabaseMetaData* md=con->getMetaData();
    cout << "Connected to " 
	 << md->getDatabaseProductName() << " "
	 << md->getDatabaseProductVersion()
	 << " using " << md->getDriverName() << " "
	 << md->getDriverVersion() << " (ODBC Version "
	 << md->getDriverMajorVersion() << "."
	 << md->getDriverMinorVersion() << ")" << endl;


    {
      Isql isql(con);
      isql.run();
    }

    cout << endl;
  } catch(SQLException& e) {
    cerr << endl << "Whoops: " << e.getMessage() << endl;
    return 1;
  }

  return 0;
}
