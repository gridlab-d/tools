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

/* Bring in long long macros needed by unixODBC */
#if !defined(__WIN32__) && !defined(WIN32)
# include <config.h>
#endif

#include <odbc++/types.h>

#include "datahandler.h"
#include "dtconv.h"

using namespace odbc;
using namespace std;

typedef ODBC3_C(SQL_DATE_STRUCT,DATE_STRUCT) DateStruct;
typedef ODBC3_C(SQL_TIME_STRUCT,TIME_STRUCT) TimeStruct;
typedef ODBC3_C(SQL_TIMESTAMP_STRUCT,TIMESTAMP_STRUCT) TimestampStruct;

#define C_TIMESTAMP ODBC3_C(SQL_C_TYPE_TIMESTAMP,SQL_C_TIMESTAMP)
#define C_TIME ODBC3_C(SQL_C_TYPE_TIME,SQL_C_TIME)
#define C_DATE ODBC3_C(SQL_C_TYPE_DATE,SQL_C_DATE)

namespace odbc {
  const ODBCXX_CHAR_TYPE* nameOfSQLType(int sqlType) {
    static struct {
      int id;
      const ODBCXX_CHAR_TYPE* name;
    } sqlTypes[] = {
      { Types::BIGINT,        ODBCXX_STRING_CONST("BIGINT") },
      { Types::BINARY,        ODBCXX_STRING_CONST("BINARY") },
      { Types::BIT,           ODBCXX_STRING_CONST("BIT") },
      { Types::CHAR,          ODBCXX_STRING_CONST("CHAR") },
      { Types::DATE,          ODBCXX_STRING_CONST("DATE") },
      { Types::DECIMAL,       ODBCXX_STRING_CONST("DECIMAL") },
      { Types::DOUBLE,        ODBCXX_STRING_CONST("DOUBLE") },
      { Types::FLOAT,         ODBCXX_STRING_CONST("FLOAT") },
      { Types::INTEGER,       ODBCXX_STRING_CONST("INTEGER") },
      { Types::LONGVARBINARY, ODBCXX_STRING_CONST("LONGVARBINARY") },
      { Types::LONGVARCHAR,   ODBCXX_STRING_CONST("LONGVARCHAR") },
      { Types::NUMERIC,       ODBCXX_STRING_CONST("NUMERIC") },
      { Types::REAL,          ODBCXX_STRING_CONST("REAL") },
      { Types::SMALLINT,      ODBCXX_STRING_CONST("SMALLINT") },
      { Types::TIME,          ODBCXX_STRING_CONST("TIME") },
      { Types::TIMESTAMP,     ODBCXX_STRING_CONST("TIMESTAMP") },
      { Types::TINYINT,       ODBCXX_STRING_CONST("TINYINT") },
      { Types::VARBINARY,     ODBCXX_STRING_CONST("VARBINARY") },
      { Types::VARCHAR,       ODBCXX_STRING_CONST("VARCHAR") },
#ifdef ODBCXX_HAVE_STRUCT_GUID
    { Types::GUID,          ODBCXX_STRING_CONST("GUID") },
#endif
      { 0,                    NULL }
    };

    for(unsigned int i=0; sqlTypes[i].name!=NULL; i++) {
      if(sqlTypes[i].id==sqlType) {
        return sqlTypes[i].name;
      }
    }

    return ODBCXX_STRING_CONST("UNKNOWN");
  }

  const ODBCXX_CHAR_TYPE* nameOfCType(int cType) {
    static struct {
      int id;
      const ODBCXX_CHAR_TYPE* name;
    } cTypes[] = {
#if defined(ODBCXX_HAVE_SQLUCODE_H)
      { SQL_C_WCHAR,          ODBCXX_STRING_CONST("SQL_C_WCHAR") },
#endif
      { SQL_C_CHAR,           ODBCXX_STRING_CONST("SQL_C_CHAR") },
      { SQL_C_BINARY,         ODBCXX_STRING_CONST("SQL_C_BINARY") },
      { SQL_C_BIT,            ODBCXX_STRING_CONST("SQL_C_BIT") },
      { SQL_C_TINYINT,        ODBCXX_STRING_CONST("SQL_C_TINYINT") },
      { SQL_C_SHORT,          ODBCXX_STRING_CONST("SQL_C_SHORT") },
      { SQL_C_LONG,           ODBCXX_STRING_CONST("SQL_C_LONG") },
      { SQL_C_DOUBLE,         ODBCXX_STRING_CONST("SQL_C_DOUBLE") },
      { SQL_C_FLOAT,          ODBCXX_STRING_CONST("SQL_C_FLOAT") },
      { SQL_C_DATE,           ODBCXX_STRING_CONST("SQL_C_DATE") },
      { SQL_C_TIME,           ODBCXX_STRING_CONST("SQL_C_TIME") },
      { SQL_C_TIMESTAMP,      ODBCXX_STRING_CONST("SQL_C_TIMESTAMP") },
#if ODBCVER >= 0x0300
      { SQL_C_SBIGINT,        ODBCXX_STRING_CONST("SQL_C_SBIGINT") },
      { SQL_C_TYPE_TIME,      ODBCXX_STRING_CONST("SQL_C_TYPE_TIME") },
      { SQL_C_TYPE_DATE,      ODBCXX_STRING_CONST("SQL_C_TYPE_DATE") },
      { SQL_C_TYPE_TIMESTAMP, ODBCXX_STRING_CONST("SQL_C_TYPE_TIMESTAMP") },
#endif
      { 0,                        NULL }
    };

    for(unsigned int i=0; cTypes[i].name!=NULL; i++) {
      if(cTypes[i].id==cType) {
        return cTypes[i].name;
      }
    }
    return ODBCXX_STRING_CONST("UNKNOWN");
  }

};

#define UNSUPPORTED_GET(as_type)                                \
throw SQLException                                                \
(ODBCXX_STRING_CONST("[libodbc++]: Could not get SQL type ")+ \
 intToString(sqlType_)+ODBCXX_STRING_CONST(" (")+ \
 nameOfSQLType(sqlType_)+ODBCXX_STRING_CONST("), C type ")+ \
 intToString(cType_)+ODBCXX_STRING_CONST(" (")+ \
 nameOfCType(cType_)+ODBCXX_STRING_CONST(") as ") as_type, SQLException::scDEFSQLSTATE)

#define UNSUPPORTED_SET(to_type)                                \
throw SQLException                                                \
(ODBCXX_STRING_CONST("[libodbc++]: Could not set SQL type ")+ \
 intToString(sqlType_)+ ODBCXX_STRING_CONST(" (")+ \
 nameOfSQLType(sqlType_)+ODBCXX_STRING_CONST("), C type ")+ \
 intToString(cType_)+ODBCXX_STRING_CONST(" (")+ \
 nameOfCType(cType_)+ODBCXX_STRING_CONST(") to ") to_type, SQLException::scDEFSQLSTATE)





void DataHandler::setupBuffer(size_t s)
{
  if(bufferSize_>0 && buffer_ != NULL) 
  {//    delete[] buffer_;
	  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__)[] buffer_;
  }

  bufferSize_=s;
  if(bufferSize_>0) {
    buffer_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) char[rows_*bufferSize_];
  } else {
    buffer_=NULL;
  }
}


DataHandler::DataHandler(unsigned int& cr, size_t rows,
                         int sqlType, int precision, int scale,
                         bool use3)
  :currentRow_(cr),rows_(rows),
   buffer_(NULL),bufferSize_(0), dataStatus_(NULL),
   isStreamed_(false),stream_(NULL),ownStream_(false),
   sqlType_(sqlType),
   precision_(precision),
   scale_(scale),
   use3_(use3)
#ifdef CACHE_STREAMED_DATA
#endif // CACHE_STREAMED_DATA
{

  size_t bs=0;

  switch(sqlType_) {
  case Types::CHAR:
  case Types::VARCHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case Types::WVARCHAR:
  case Types::WCHAR:
    cType_=SQL_C_TCHAR;
#else
    cType_=SQL_C_CHAR;
#endif
    scale_=0;
    bs=(precision_+1)*sizeof(ODBCXX_CHAR_TYPE); //string+null
    break;

  case Types::NUMERIC:
  case Types::DECIMAL:
    cType_=SQL_C_CHAR;
    bs=precision_+3; //sign,comma,null
    break;

  case Types::BIGINT:
    // for ODBC3, we fetch this as an SQLBIGINT
    // for ODBC2, we convert to a string
#if ODBCVER >= 0x0300
    if(use3_) {
      cType_=SQL_C_SBIGINT;
      scale_=0;
      bs=sizeof(SQLBIGINT);
      break;
    }
#endif
    cType_=SQL_C_CHAR;
    scale_=0;
    bs=21; //19 digits, sign,null
    break;

  case Types::BIT:
    cType_=SQL_C_BIT;
    scale_=0;
    bs=sizeof(signed char);
    break;

  case Types::TINYINT:
    cType_=SQL_C_TINYINT;
    scale_=0;
    bs=sizeof(signed char);
    break;

  case Types::SMALLINT:
    cType_=SQL_C_SHORT;
    scale_=0;
    bs=sizeof(short);
    break;

  case Types::INTEGER:
    cType_=SQL_C_LONG;
    scale_=0;
    bs=sizeof(SQLINTEGER);
    break;

  case Types::FLOAT:
  case Types::DOUBLE:
    cType_=SQL_C_DOUBLE;
    bs=sizeof(double);
    break;

  case Types::REAL:
    cType_=SQL_C_FLOAT;
    bs=sizeof(float);
    break;

  case Types::BINARY:
  case Types::VARBINARY:
    cType_=SQL_C_BINARY;
    bs=precision_;
    break;

  case Types::DATE:
    cType_=C_DATE;
    bs=sizeof(DateStruct);
    break;

  case Types::TIME:
    cType_=C_TIME;
    bs=sizeof(TimeStruct);
    break;

  case Types::TIMESTAMP:
    cType_=C_TIMESTAMP;
    bs=sizeof(TimestampStruct);
    break;

  case Types::LONGVARCHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case Types::WLONGVARCHAR:
    cType_=SQL_C_TCHAR;
#else
    cType_=SQL_C_CHAR;
#endif
    bs=0; // this one is streamed
    isStreamed_=true;
    break;

  case Types::LONGVARBINARY:
    cType_=SQL_C_BINARY;
    bs=0; // same here
    isStreamed_=true;
	break;
		  
#ifdef ODBCXX_HAVE_STRUCT_GUID
  case Types::GUID:
	  cType_ = SQL_C_GUID;
	  bs=36;
	  break;
#endif		  

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: DataHandler: unhandled SQL type ")+intToString(sqlType_), SQLException::scDEFSQLSTATE);

  };
  this->setupBuffer(bs);


  dataStatus_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DATASTATUS_TYPE[rows_];
  //set everything to NULL
  for(unsigned int i=0; i<rows_; i++) {
    dataStatus_[i]=SQL_NULL_DATA;
  }
}


#define GET_AS(type) (CURRENT_RETTYPE)*(type*)this->data()

#define ACCEPT_GET(id,type) case id: return GET_AS(type)

bool DataHandler::getBoolean() const
{
  return this->getInt()!=0;
}

signed char DataHandler::getByte() const
{
  return (signed char)this->getInt();
}

short DataHandler::getShort() const
{
  return (short)this->getInt();
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE int

int DataHandler::getInt() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return stringToInt(this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("an int"));
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE Long

Long DataHandler::getLong() const
{
  if(!this->isNull()) {
    switch(cType_) {
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return stringToLong(this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Long"));
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE float

float DataHandler::getFloat() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_FLOAT,float);
      ACCEPT_GET(SQL_C_DOUBLE,double);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return (float)stringToDouble(this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a float"));
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE
#define CURRENT_RETTYPE double

double DataHandler::getDouble() const
{
  if(!this->isNull()) {
    switch(cType_) {
      ACCEPT_GET(SQL_C_DOUBLE,double);
      ACCEPT_GET(SQL_C_FLOAT,float);
#if ODBCVER >= 0x0300
      ACCEPT_GET(SQL_C_SBIGINT,SQLBIGINT);
#endif
      ACCEPT_GET(SQL_C_LONG,SQLINTEGER);
      ACCEPT_GET(SQL_C_SHORT,short);
      ACCEPT_GET(SQL_C_TINYINT,signed char);
      ACCEPT_GET(SQL_C_BIT,signed char);

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return (double)stringToDouble(this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a double"));
    }
  }

  return 0;
}

#undef CURRENT_RETTYPE

Date DataHandler::getDate() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_DATE:
      {
        DateStruct* ds=(DateStruct*)this->data();
        return Date(ds->year,ds->month,ds->day);
      }

    case C_TIMESTAMP:
      {
        TimestampStruct* ts=(TimestampStruct*)this->data();
        return Date(ts->year,ts->month,ts->day);
      }

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return Date((ODBCXX_CHAR_TYPE*)this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Date"));
    }
  }

  return Date();
}

#ifdef ODBCXX_HAVE_STRUCT_GUID
Guid DataHandler::getGuid() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case SQL_C_GUID:
        return Guid((ODBCXX_SIGNED_CHAR_TYPE*)this->data(), this->getDataStatus());
    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Guid"));
    }
  }
  return Guid();
}
#endif

Time DataHandler::getTime() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_TIME:
      {
        TimeStruct* ts=(TimeStruct*)this->data();
        return Time(ts->hour,ts->minute,ts->second);
      }

    case C_TIMESTAMP:
      {
        TimestampStruct* ts=(TimestampStruct*)this->data();
        return Time(ts->hour,ts->minute,ts->second);
      }

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return Time((ODBCXX_CHAR_TYPE*)this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Time"));
    }
  }

  return Time();
}

Timestamp DataHandler::getTimestamp() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case C_TIMESTAMP:
      {
        TimestampStruct* ts=(TimestampStruct*)this->data();
        return Timestamp(ts->year,ts->month,ts->day,
                         ts->hour,ts->minute,ts->second,ts->fraction);
      }

    case C_DATE:
      {
        DateStruct* ds=(DateStruct*)this->data();
        return Timestamp(ds->year,ds->month,ds->day,
                         0,0,0);
      }

    case C_TIME:
      {
        TimeStruct* ts=(TimeStruct*)this->data();
        return Timestamp(0,0,0,ts->hour,ts->minute,ts->second);
      }

    case SQL_C_CHAR:
      if(!isStreamed_) {
        return Timestamp((ODBCXX_CHAR_TYPE*)this->data());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Timestamp"));
    }
  }

  return Timestamp();
}


ODBCXX_STRING DataHandler::getString() const
{
  if(!this->isNull()) {
    switch(cType_) {
#if defined(ODBCXX_HAVE_SQLUCODE_H)
    case SQL_C_TCHAR:
#else
    case SQL_C_CHAR:
#endif
      if(!isStreamed_) {
        switch(this->getDataStatus()) {
           case SQL_NTS:
             return ODBCXX_STRING_C((ODBCXX_CHAR_TYPE*)this->data());
           case SQL_NO_TOTAL:
             return ODBCXX_STRING_CL((ODBCXX_CHAR_TYPE*)this->data(),
                                     bufferSize_/sizeof(ODBCXX_CHAR_TYPE));
           default:
             return ODBCXX_STRING_CL((ODBCXX_CHAR_TYPE*)this->data(),
                                     this->getDataStatus()/sizeof(ODBCXX_CHAR_TYPE));
        }
      } else {
        throw SQLException(ODBCXX_STRING_CONST("[libodbc++]: NYI: Getting a stream as a string"), SQLException::scDEFSQLSTATE);
      }

    case C_DATE:
      return this->getDate().toString();

#ifdef ODBCXX_HAVE_STRUCT_GUID
    case SQL_C_GUID:
      return this->getGuid().toString();
#endif

    case C_TIME:
      return this->getTime().toString();

    case C_TIMESTAMP:
      return this->getTimestamp().toString();

    case SQL_C_BIT:
    case SQL_C_TINYINT:
    case SQL_C_SHORT:
    case SQL_C_LONG:
      return intToString(this->getInt());

#if ODBCVER >= 0x0300
    case SQL_C_SBIGINT:
      return longToString(this->getLong());
#endif

    case SQL_C_FLOAT:
    case SQL_C_DOUBLE:
      return doubleToString(this->getDouble());

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a string"));
    }
  }

  return ODBCXX_STRING(ODBCXX_STRING_CONST(""));
}


ODBCXX_BYTES DataHandler::getBytes() const
{
  if(!this->isNull()) {
    switch(cType_) {
    case SQL_C_CHAR:
    case SQL_C_BINARY:
#if defined(ODBCXX_HAVE_SQLUCODE_H) && defined(ODBCXX_UNICODE)
    case SQL_C_WCHAR :
#endif
      if(!isStreamed_) {
        return ODBCXX_BYTES_C(this->data(),this->getDataStatus());
      }

    default:
      UNSUPPORTED_GET(ODBCXX_STRING_CONST("a Bytes"));
    }
  }

  return ODBCXX_BYTES_C(NULL,0);
}


ODBCXX_STREAM* DataHandler::getStream() const
{
  // in here, we can't trust this->isNull(), since
  // this probably isn't bound.
  switch(cType_) {
  case SQL_C_BINARY:
  case SQL_C_CHAR:
#if defined(ODBCXX_HAVE_SQLUCODE_H) && defined(ODBCXX_UNICODE)
  case SQL_C_WCHAR:
#endif
    if(isStreamed_) {
      return stream_;
    }

  default:
    UNSUPPORTED_GET(ODBCXX_STRING_CONST("a stream"));
  }

  // notreached
  assert(false);
  ODBCXX_DUMMY_RETURN(NULL);
}

#define SET_TO(type,val)                        \
*(type*)this->data()=(type)val;                        \
this->setDataStatus(sizeof(type))


#define ACCEPT_SET_VAL(id,type,val)                \
case id: SET_TO(type,val); break

#define ACCEPT_SET(id,type) ACCEPT_SET_VAL(id,type,val)

void DataHandler::setBoolean(bool b)
{
  this->setInt(b?1:0);
}

void DataHandler::setByte(signed char b)
{
  this->setInt(b);
}

void DataHandler::setShort(short s)
{
  this->setInt(s);
}

void DataHandler::setInt(int val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif

    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);

  case SQL_C_CHAR:
    this->setString(intToString(val));
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("an int"));
  }
}

void DataHandler::setLong(Long val)
{
  switch(cType_) {
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif

    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);
    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);

  case SQL_C_CHAR:
    this->setString(longToString(val));
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a Long"));
  }
}

void DataHandler::setFloat(float val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_FLOAT,float);
    ACCEPT_SET(SQL_C_DOUBLE,double);
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

  case SQL_C_CHAR:
    this->setString(doubleToString(val));
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a float"));
  }
}

void DataHandler::setDouble(double val)
{
  switch(cType_) {
    ACCEPT_SET(SQL_C_DOUBLE,double);
    ACCEPT_SET(SQL_C_FLOAT,float);
#if ODBCVER >= 0x0300
    ACCEPT_SET(SQL_C_SBIGINT,SQLBIGINT);
#endif
    ACCEPT_SET(SQL_C_BIT,signed char);
    ACCEPT_SET(SQL_C_TINYINT,signed char);
    ACCEPT_SET(SQL_C_SHORT,short);
    ACCEPT_SET(SQL_C_LONG,SQLINTEGER);

  case SQL_C_CHAR:
    this->setString(doubleToString(val));
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a double"));
  }
}


void DataHandler::setDate(const Date& d)
{
  switch(cType_) {
  case C_DATE:
    {
      DateStruct* ds=(DateStruct*)this->data();
      ds->year=d.getYear();
      ds->month=d.getMonth();
      ds->day=d.getDay();
      this->setDataStatus(sizeof(DateStruct));
    }
    break;

  case C_TIMESTAMP:
    {
      TimestampStruct* ts=(TimestampStruct*)this->data();
      ts->year=d.getYear();
      ts->month=d.getMonth();
      ts->day=d.getDay();
      ts->hour=0;
      ts->minute=0;
      ts->second=0;
      ts->fraction=0;
      this->setDataStatus(sizeof(TimestampStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      // ODBC date escape
      this->setString(ODBCXX_STRING_CONST("{d '")+d.toString()+ODBCXX_STRING_CONST("'}"));
      break;
    }

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a Date"));
  }
}

void DataHandler::setTime(const Time& t)
{
  switch(cType_) {
  case C_TIME:
    {
      TimeStruct* ts=(TimeStruct*)this->data();
      ts->hour=t.getHour();
      ts->minute=t.getMinute();
      ts->second=t.getSecond();
      this->setDataStatus(sizeof(TimeStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      this->setString(ODBCXX_STRING_CONST("{t '")+t.toString()+ODBCXX_STRING_CONST("'}"));
      break;
    }

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a Time"));
  }
}

void DataHandler::setTimestamp(const Timestamp& t)
{
  switch(cType_) {
  case C_TIMESTAMP:
    {
      TimestampStruct* ts=(TimestampStruct*)this->data();
      ts->year=t.getYear();
      ts->month=t.getMonth();
      ts->day=t.getDay();
      ts->hour=t.getHour();
      ts->minute=t.getMinute();
      ts->second=t.getSecond();
      ts->fraction=t.getNanos();
      this->setDataStatus(sizeof(TimestampStruct));
    }
    break;

  case SQL_C_CHAR:
    if(!isStreamed_) {
      this->setString(ODBCXX_STRING_CONST("{ts '")+t.toString()+ODBCXX_STRING_CONST("'}"));
      break;
    }

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a Timestamp"));
  }
}


void DataHandler::setString(const ODBCXX_STRING& str)
{
  switch(cType_) {
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case SQL_C_TCHAR:
#else
  case SQL_C_CHAR:
#endif
    if(!isStreamed_) {
      unsigned int len=(unsigned int)ODBCXX_STRING_LEN(str);
      if((len+1)*sizeof(ODBCXX_CHAR_TYPE)>bufferSize_) {
        len=(bufferSize_-1)/sizeof(ODBCXX_CHAR_TYPE);
      }
      ODBCXX_CHAR_TYPE* buf=(ODBCXX_CHAR_TYPE*)this->data();
      // we want to pad (W)CHARs with spaces
      unsigned int padlen=(sqlType_==Types::CHAR
#if defined(ODBCXX_HAVE_SQLUCODE_H)
         ||sqlType_==Types::WCHAR
#endif
         ?(bufferSize_-1)/sizeof(ODBCXX_CHAR_TYPE)-len:0);

      memcpy(buf,ODBCXX_STRING_DATA(str),len*sizeof(ODBCXX_CHAR_TYPE));
      for(unsigned int i=0; i<padlen; i++) {
        buf[len+i]=' ';
      }

      buf[len+padlen]='\0'; //NULL

      this->setDataStatus((len+padlen)*sizeof(ODBCXX_CHAR_TYPE));
    } else {
      // we fake a real setStream()
      this->setStream(stringToStream(str),
                      ODBCXX_STRING_LEN(str));
      ownStream_=true;
    }
    break;

    ACCEPT_SET_VAL(SQL_C_BIT,signed char,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_TINYINT,signed char,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_SHORT,short,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_LONG,SQLINTEGER,stringToInt(str));
    ACCEPT_SET_VAL(SQL_C_DOUBLE,double,stringToDouble(str));
    ACCEPT_SET_VAL(SQL_C_FLOAT,float,stringToDouble(str));

  case C_DATE:
    this->setDate(Date(str));
    break;

  case C_TIME:
    this->setTime(Time(str));
    break;

  case C_TIMESTAMP:
    this->setTimestamp(Timestamp(str));
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a string"));
  }
}

void DataHandler::setBytes(const ODBCXX_BYTES& b)
{
  switch(cType_) {
  case SQL_C_BINARY:
#ifdef ODBCXX_HAVE_STRUCT_GUID
  case SQL_GUID:
#endif
    if(!isStreamed_) {
      size_t l=ODBCXX_BYTES_SIZE(b);
      // truncate if needed
      if(l>bufferSize_) {
        l=bufferSize_;
      }

      memcpy(this->data(),ODBCXX_BYTES_DATA(b),l);
      this->setDataStatus(l);

    } else {
      // fake a setStream()
      this->setStream(bytesToStream(b),
                      ODBCXX_BYTES_SIZE(b));
      ownStream_=true;
#ifdef CACHE_STREAMED_DATA
#endif // CACHE_STREAMED_DATA
    }
    break;

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a const Bytes&"));
  }
}

#ifdef ODBCXX_HAVE_STRUCT_GUID
void DataHandler::setGuid(const Guid& g) {
    Bytes data(g.getData(), g.getSize());
    setBytes(data);
}
#endif

void DataHandler::setStream(ODBCXX_STREAM* s)
{
  this->resetStream();
  stream_=s;
  ownStream_=true;
#ifdef CACHE_STREAMED_DATA
#endif // CACHE_STREAMED_DATA
}

void DataHandler::setStream(ODBCXX_STREAM* s, int len, bool ownstream)
{
  switch(cType_) {
  case SQL_C_CHAR:
  case SQL_C_BINARY:
    if(isStreamed_) {
      this->resetStream();
      stream_=s;
      ownStream_=ownstream;
      this->setDataStatus(SQL_LEN_DATA_AT_EXEC(len*sizeof(ODBCXX_CHAR_TYPE)));
      break;
    }

  default:
    UNSUPPORTED_SET(ODBCXX_STRING_CONST("a stream"));
  }
#ifdef CACHE_STREAMED_DATA
#endif // CACHE_STREAMED_DATA
}
