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

#include <odbc++/callablestatement.h>

#include "datahandler.h"
#include "dtconv.h"

using namespace odbc;
using namespace std;

CallableStatement::CallableStatement(Connection* con,
				     SQLHSTMT hstmt,
				     const ODBCXX_STRING& sql,
				     int resultSetType,
				     int resultSetConcurrency)
  :PreparedStatement(con,hstmt,sql,
		     resultSetType,resultSetConcurrency,
		     SQL_PARAM_INPUT_OUTPUT),
   lastWasNull_(false)
{
  // override defaultDirection_, which PreparedStatement
  // sets to SQL_PARAM_INPUT
  defaultDirection_=SQL_PARAM_INPUT_OUTPUT;
}

CallableStatement::~CallableStatement()
{
}

void CallableStatement::registerInParameter(int idx)
{
  directions_[idx-1]=SQL_PARAM_INPUT;
}


void CallableStatement::registerOutParameter(int idx, int sqlType, int scale)
{
  int defPrec=DataHandler::defaultPrecisionFor(sqlType);
  this->_checkParam(idx,&sqlType,1,defPrec,scale);
  directions_[idx-1]=SQL_PARAM_OUTPUT;
}


#define CHECK_COL(idx,FUNCSUFFIX)							\
do {											\
  if(idx<1 || idx>(int)numParams_) {							\
    throw SQLException									\
      (ODBCXX_STRING_CONST("[libodbc++]: PreparedStatement::set") \
       ODBCXX_STRING_CONST(#FUNCSUFFIX) \
       ODBCXX_STRING_CONST("(): parameter index ")+	\
       intToString(idx)+ODBCXX_STRING_CONST(" out of range"), ODBCXX_STRING_CONST("S1093"));						\
  }											\
} while(false)



#define IMPLEMENT_GET(RETTYPE,FUNCSUFFIX)			\
RETTYPE CallableStatement::get##FUNCSUFFIX(int idx)		\
{								\
  CHECK_COL(idx,FUNCSUFFIX);					\
  DataHandler* dh=rowset_->getColumn(idx);			\
  lastWasNull_=dh->isNull();					\
  return dh->get##FUNCSUFFIX();					\
}								\

IMPLEMENT_GET(double,Double);
IMPLEMENT_GET(bool,Boolean);
IMPLEMENT_GET(signed char,Byte);
IMPLEMENT_GET(ODBCXX_BYTES,Bytes);
IMPLEMENT_GET(Date,Date);
IMPLEMENT_GET(float,Float);
IMPLEMENT_GET(int,Int);
IMPLEMENT_GET(Long,Long);
IMPLEMENT_GET(short,Short);
IMPLEMENT_GET(Time,Time);
IMPLEMENT_GET(Timestamp,Timestamp);
IMPLEMENT_GET(ODBCXX_STRING,String);
