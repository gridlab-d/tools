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

#include <odbc++/types.h>
#include <odbc++/threads.h>

#include "dtconv.h"

#include <cstring>

#include <cstdlib>
#include <cstdio>

// lie to stuff below
#if !defined(ODBCXX_HAVE_SNPRINTF) && defined(ODBCXX_HAVE__SNPRINTF)
# define snprintf _snprintf
# define ODBCXX_HAVE_SNPRINTF 1
#endif

using namespace odbc;
using namespace std;

void Time::_invalid(const ODBCXX_CHAR_TYPE* what, int value)
{
  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Invalid TIME: ");
  msg+=what+ODBCXX_STRING(ODBCXX_STRING_CONST(" out of range ("))
     +intToString(value)+ODBCXX_STRING_CONST(")");
  throw SQLException(msg, ODBCXX_STRING_CONST("22007"));
}

void Date::_invalid(const ODBCXX_CHAR_TYPE* what, int value)
{
  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Invalid DATE: ");
  msg+=what+ODBCXX_STRING(ODBCXX_STRING_CONST(" out of range ("))
     +intToString(value)+ODBCXX_STRING_CONST(")");
  throw SQLException(msg, ODBCXX_STRING_CONST("22007"));
}

void Timestamp::_invalid(const ODBCXX_CHAR_TYPE* what, int value)
{
  ODBCXX_STRING msg=ODBCXX_STRING_CONST("Invalid TIMESTAMP: ");
  msg+=what+ODBCXX_STRING(ODBCXX_STRING_CONST(" out of range ("))
     +intToString(value)+ODBCXX_STRING_CONST(")");
  throw SQLException(msg, ODBCXX_STRING_CONST("22007"));
}

namespace odbc {

  /* wrapper for localtime */
  inline void localtime(time_t t, tm* stm) {

#if defined(ODBCXX_ENABLE_THREADS)

# if defined(ODBCXX_HAVE_LOCALTIME_R)

    ::localtime_r(&t,stm);

# else

    // workaround
    static odbc::Mutex lock;
    {
      ODBCXX_LOCKER(lock);
      memcpy(stm,::localtime(&t),sizeof(tm));
    }


# endif // HAVE_LOCALTIME_R

#else
    // no reentrancy needed
    memcpy(stm,::localtime(&t),sizeof(tm));
#endif
  }
};


Date::Date()
{
  this->setTime(time(NULL));
}

void Date::setTime(time_t t)
{
  tm stm;
  odbc::localtime(t, &stm);
  this->setYear(stm.tm_year+1900);
  this->setMonth(stm.tm_mon+1);
  this->setDay(stm.tm_mday);
}

time_t Date::getTime() const
{
  tm stm;
  stm.tm_year=year_-1900;
  stm.tm_mon=month_-1;
  stm.tm_mday=day_;
  stm.tm_hour=0;
  stm.tm_min=0;
  stm.tm_sec=0;
  stm.tm_isdst=-1; //negative means not known
  return mktime(&stm);
}

ODBCXX_STRING Date::toString() const
{
  ODBCXX_CHAR_TYPE buf[11]; // YYYY-MM-DD, null
#if defined(ODBCXX_HAVE__SNPRINTF)
# if defined(ODBCXX_UNICODE)
    _snwprintf(buf,11
# else
    _snprintf(buf,11
# endif
#elif defined(ODBCXX_HAVE_SNPRINTF) && !defined(ODBCXX_UNICODE)
    snprintf(buf,11
#else
# if defined(ODBCXX_UNICODE)
    swprintf(buf,11
# else
    sprintf(buf
# endif
#endif
          ,ODBCXX_STRING_CONST("%.4d-%.2d-%.2d"),
          year_,month_,day_);
  return ODBCXX_STRING(buf);
}

void Date::parse(const ODBCXX_STRING& in)
{
  if(ODBCXX_STRING_LEN(in)!=10) {
    throw SQLException(ODBCXX_STRING_CONST("[libodbc++]: Unrecognized date format: ")+in, ODBCXX_STRING_CONST("22007"));
  }

  ODBCXX_CHAR_TYPE buf[11];
#if defined(ODBCXX_UNICODE)
  wcscpy
#else
  strcpy
#endif
    (buf,ODBCXX_STRING_CSTR(in));
  buf[4]=0;
  buf[7]=0;

  this->setYear(ODBCXX_STRTOL(buf,NULL,10));
  this->setMonth(ODBCXX_STRTOL(&buf[5],NULL,10));
  this->setDay(ODBCXX_STRTOL(&buf[8],NULL,10));
}


Time::Time()
{
  this->setTime(time(NULL));
}

void Time::setTime(time_t t)
{
  struct tm stm;
  odbc::localtime(t,&stm);
  this->setHour(stm.tm_hour);
  this->setMinute(stm.tm_min);
  this->setSecond(stm.tm_sec);
}

time_t Time::getTime() const
{
  return second_+minute_*60+hour_*3600;
}

ODBCXX_STRING Time::toString() const
{
  ODBCXX_CHAR_TYPE buf[9]; // HH:MM:SS, null
#if defined(ODBCXX_HAVE__SNPRINTF)
# if defined(ODBCXX_UNICODE)
    _snwprintf(buf,9
# else
    _snprintf(buf,9
# endif
#elif defined(ODBCXX_HAVE_SNPRINTF) && !defined(ODBCXX_UNICODE)
    snprintf(buf,9
#else
# if defined(ODBCXX_UNICODE)
    swprintf(buf,9
# else
    sprintf(buf
# endif
#endif
          ,ODBCXX_STRING_CONST("%.2d:%.2d:%.2d"),
          hour_,minute_,second_);
  return ODBCXX_STRING_C(buf);
}

void Time::parse(const ODBCXX_STRING& in)
{
  if(ODBCXX_STRING_LEN(in)!=8) {
    throw SQLException(ODBCXX_STRING_CONST("Unrecognized time format: ")+in, ODBCXX_STRING_CONST("22007"));
  }

  ODBCXX_CHAR_TYPE buf[9];
#if defined(ODBCXX_UNICODE)
  wcscpy
#else
  strcpy
#endif
    (buf,ODBCXX_STRING_CSTR(in));
  buf[2]=0;
  buf[5]=0;

  this->setHour(ODBCXX_STRTOL(buf,NULL,10));
  this->setMinute(ODBCXX_STRTOL(&buf[3],NULL,10));
  this->setSecond(ODBCXX_STRTOL(&buf[6],NULL,10));
}



Timestamp::Timestamp()
{
  this->setTime(time(NULL));
}

void Timestamp::setTime(time_t t)
{
  struct tm stm;
  odbc::localtime(t,&stm);

  this->setYear(stm.tm_year+1900);
  this->setMonth(stm.tm_mon+1);
  this->setDay(stm.tm_mday);
  this->setHour(stm.tm_hour);
  this->setMinute(stm.tm_min);
  this->setSecond(stm.tm_sec);

  nanos_=0;
}

ODBCXX_STRING Timestamp::toString() const
{
  ODBCXX_STRING ret=Date::toString()+ODBCXX_STRING_CONST(" ")+Time::toString();
  if(nanos_>0) {
    ret+=ODBCXX_STRING_CONST(".");
    ODBCXX_CHAR_TYPE buf[10];
#if defined(ODBCXX_HAVE__SNPRINTF)
# if defined(ODBCXX_UNICODE)
    _snwprintf(buf,9
# else
    _snprintf(buf,9
# endif
#elif defined(ODBCXX_HAVE_SNPRINTF) && !defined(ODBCXX_UNICODE)
    snprintf(buf,9
#else
# if defined(ODBCXX_UNICODE)
    swprintf(buf,9
# else
    sprintf(buf
# endif
#endif
      ,ODBCXX_STRING_CONST("%09d"),nanos_);
      buf[9] = NULL;// Terminate string
    ret+=ODBCXX_STRING(buf);
  }
  return ret;
}

void Timestamp::parse(const ODBCXX_STRING& in)
{
  // YYYY-MM-DD HH:MM:SS.xxxxxxxxxx (max 30 chars)
  if(ODBCXX_STRING_LEN(in)<19 ||
     ODBCXX_STRING_LEN(in)>30) {
    throw SQLException(ODBCXX_STRING_CONST("Unrecognized timestamp format: ")+in, ODBCXX_STRING_CONST("22007"));
  }

  ODBCXX_CHAR_TYPE buf[31];
#if defined(ODBCXX_UNICODE)
  wcscpy
#else
  strcpy
#endif
    (buf,ODBCXX_STRING_CSTR(in));

  buf[4]=0;
  buf[7]=0;
  buf[10]=0;
  buf[13]=0;
  buf[16]=0;
  buf[19]=0;

  this->setYear(  ODBCXX_STRTOL(buf,     NULL,10));
  this->setMonth( ODBCXX_STRTOL(&buf[5], NULL,10));
  this->setDay(   ODBCXX_STRTOL(&buf[8], NULL,10));
  this->setHour(  ODBCXX_STRTOL(&buf[11],NULL,10));
  this->setMinute(ODBCXX_STRTOL(&buf[14],NULL,10));
  this->setSecond(ODBCXX_STRTOL(&buf[17],NULL,10));

  if(in.length()>20) {
    this->setNanos(ODBCXX_STRTOL(&buf[20],NULL,10));
  } else {
    nanos_=0;
  }
}
