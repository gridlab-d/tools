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

#ifndef __DTCONV_H
#define __DTCONV_H

#include <odbc++/types.h>

using namespace std;

#include <cstdio>
#include <cstdlib>

#if defined(ODBCXX_QT)
# include <qiodevice.h>
# include <qbuffer.h>
#else
# if defined(ODBCXX_HAVE_SSTREAM)
#  include <sstream>
# else
#  include <strstream>
# endif
#endif
/* conversion functions string <-> int/float/double/long double
 */

// This is a pure mess, but seems to function (tm).

namespace odbc {

  const int INT_STR_LEN=12; //10 digits, sign, null
  const int LONG_STR_LEN=22; //20 digits, sign, null
  const int DOUBLE_STR_LEN=80; //???


  inline ODBCXX_STRING intToString(int i) {

#if defined(ODBCXX_QT)

    return QString::number(i);

#else

    ODBCXX_CHAR_TYPE buf[INT_STR_LEN];

# if defined(WIN32) && defined(ODBCXX_HAVE__ITOA)
#  if defined(ODBCXX_UNICODE)
    _itow(i,buf,10);
#  else
    _itoa(i,buf,10);
#  endif
# elif defined(WIN32) && defined(ODBCXX_HAVE_ITOA)
#  if defined(ODBCXX_UNICODE)
    itow(i,buf,10);
#  else
    itoa(i,buf,10);
#  endif
# else
#  if defined(ODBCXX_UNICODE)
    swprintf(buf,INT_STR_LEN,L"%d",i);
# else
    snprintf(buf,INT_STR_LEN,"%d",i);
# endif
# endif
    return ODBCXX_STRING(buf);
#endif
  }

  inline int stringToInt(const ODBCXX_STRING& s) {
#if defined(ODBCXX_QT)
    return s.toInt();
#else
    return (int)ODBCXX_STRTOL(s.c_str(),NULL,10);
#endif
  }

#if defined(ODBCXX_UNICODE)
  inline int stringToInt(const std::string& s) {
    return (int)strtol(s.c_str(),NULL,10);
  }
#endif

  inline ODBCXX_STRING longToString(Long l) {
    ODBCXX_CHAR_TYPE buf[LONG_STR_LEN];
#if defined(WIN32) && defined(ODBCXX_HAVE__I64TOA)
# if defined(ODBCXX_UNICODE)
    _i64tow(l,buf,10);
# else
    _i64toa(l,buf,10);
# endif
#else
# if defined(ODBCXX_UNICODE)
    swprintf(buf,LONG_STR_LEN,
# else
    snprintf(buf,LONG_STR_LEN,
# endif
# if defined(PRId64)
             ODBCXX_STRING_PERCENT PRId64
# elif ODBCXX_SIZEOF_LONG==8
             ODBCXX_STRING_CONST("%ld")
# else
             ODBCXX_STRING_CONST("%lld")
# endif
             ,l);
#endif // _i64toa
    return ODBCXX_STRING_C(buf);
  }

  inline odbc::Long stringToLong(const ODBCXX_STRING& _s) {
#if !defined(ODBCXX_QT)
    const ODBCXX_CHAR_TYPE* s=ODBCXX_STRING_CSTR(_s);
#else
    QCString cs(_s.local8Bit());
    const char* s=cs.data();
#endif

#if defined(WIN32) && defined(ODBCXX_HAVE__ATOI64)
# if defined(ODBCXX_UNICODE)
    return (Long)_wtoi64(s);
# else
    return (Long)_atoi64(s);
# endif
#elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOLL)
# if defined(ODBCXX_UNICODE)
    return (Long)wcstoll(s,NULL,10);
# else
    return (Long)strtoll(s,NULL,10);
# endif
#elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOQ)
# if defined(ODBCXX_UNICODE)
    return (Long)wcstoq(s,NULL,10);
# else
    return (Long)strtoq(s,NULL,10);
# endif
#else
    // either 64bit platform, or I'm stupid.
    return (Long)ODBCXX_STRTOL(s,NULL,10);
#endif
  }

#if defined(ODBCXX_UNICODE)
  inline odbc::Long stringToLong(const std::string& _s) {
    const char* s=_s.c_str();

# if defined(WIN32) && defined(ODBCXX_HAVE__ATOI64)
    return (Long)_atoi64(s);
# elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOLL)
    return (Long)strtoll(s,NULL,10);
# elif ODBCXX_SIZEOF_LONG==4 && defined(ODBCXX_HAVE_STRTOQ)
    return (Long)strtoq(s,NULL,10);
# else
    // either 64bit platform, or I'm stupid.
    return (Long)strtol(s,NULL,10);
# endif
  }
#endif

  inline ODBCXX_STRING doubleToString(double d) {
    ODBCXX_CHAR_TYPE buf[DOUBLE_STR_LEN];
#if defined(ODBCXX_HAVE__SNPRINTF)
# if defined(ODBCXX_UNICODE)
    _snwprintf(buf,DOUBLE_STR_LEN,L"%f",d);
# else
    _snprintf(buf,DOUBLE_STR_LEN,"%f",d);
# endif
#elif defined(ODBCXX_HAVE_SNPRINTF) && !defined(ODBCXX_UNICODE)
    snprintf(buf,DOUBLE_STR_LEN,"%f",d);
#else
# if defined(ODBCXX_UNICODE)
    swprintf(buf,DOUBLE_STR_LEN,L"%f",d);
# else
    sprintf(buf,"%f",d);
# endif
#endif
    return ODBCXX_STRING_C(buf);
  }

  inline double stringToDouble(const ODBCXX_STRING& s) {
#if defined(ODBCXX_QT)
    return s.toDouble();
#else
# if defined(ODBCXX_UNICODE)
    return wcstod(s.c_str(),NULL);
# else
    return strtod(s.c_str(),NULL);
# endif
#endif
  }

#if defined(ODBCXX_UNICODE)
  inline double stringToDouble(const std::string& s) {
    return strtod(s.c_str(),NULL);
  }
#endif

  // stream stuff
  // this should return <=0 on EOF, and number of bytes
  // read otherwise
  inline int readStream(ODBCXX_STREAM* s,
                        ODBCXX_CHAR_TYPE* buf,
                        unsigned int maxlen) {
#if defined(ODBCXX_QT)
    return s->readBlock(buf,maxlen);
#else
    if(*s) {
      s->read(buf,maxlen);
      return (int)s->gcount();
    } else {
      return 0;
    }
#endif
  }

  // returns a newly allocated stream
  inline ODBCXX_STREAM* stringToStream(const ODBCXX_STRING& str) {
#if !defined(ODBCXX_QT)
    ODBCXX_SSTREAM* s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_SSTREAM();
    *s << str;
    return s;
#else // defined(ODBCXX_QT)
    QBuffer* b=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) QBuffer();
    b->open(IO_WriteOnly);
    b->writeBlock(ODBCXX_STRING_CSTR(str),ODBCXX_STRING_LEN(str));
    b->close();
    b->open(IO_ReadOnly);
    return b;
#endif
  }

  // this is rather ineffective...
  inline ODBCXX_STRING streamToString(ODBCXX_STREAM* s) {
    ODBCXX_CHAR_TYPE buf[GETDATA_CHUNK_SIZE];
#if defined(ODBCXX_QT)
    int r;
    QString ret;
	if(s != NULL)
	{	while((r=s->readBlock(buf,GETDATA_CHUNK_SIZE))>0) 
		{	ret+=ODBCXX_STRING_CL(buf,r);    }
	}
#else
    ODBCXX_STRING ret;
	if(s != NULL)
	{    while(s->read(buf,GETDATA_CHUNK_SIZE) || s->gcount()) 
		 {	ret+=ODBCXX_STRING_CL(buf,s->gcount());  }
	}
#endif
    return ret;
  }

  inline ODBCXX_BYTES streamToBytes(ODBCXX_STREAM* s) {
    ODBCXX_CHAR_TYPE buf[GETDATA_CHUNK_SIZE];
    ODBCXX_CHAR_TYPE* bigbuf=NULL;
    unsigned int size=0;
#if defined(ODBCXX_QT)
    int r;
    while((r=s->readBlock(buf,GETDATA_CHUNK_SIZE))!=-1) {
      char* tmp=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) char[size+(unsigned int)r];
      if(size>0) {
        memcpy((void*)tmp,(void*)bigbuf,size);
      }
      memcpy((void*)&tmp[size],buf,r);
      delete[] bigbuf;
      bigbuf=tmp;
      size+=(unsigned int)r;
    }

    // this should take care of deleting bigbuf
    return QByteArray().assign(bigbuf,size);
#else
    while(s->read(buf,GETDATA_CHUNK_SIZE) || s->gcount()) {
      ODBCXX_CHAR_TYPE* tmp=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[size+s->gcount()];
      if(size>0) {
        memcpy((void*)tmp,(void*)bigbuf,size);
      }
      memcpy((void*)&tmp[size],buf,s->gcount());
      delete[] bigbuf;
      bigbuf=tmp;
      size+=s->gcount();
    }

    // this copies the buffer's contents
    Bytes b((ODBCXX_SIGNED_CHAR_TYPE*)bigbuf,size);
    delete[] bigbuf;
    return b;
#endif
  }

  inline ODBCXX_STREAM* bytesToStream(const ODBCXX_BYTES& b) {
#if !defined(ODBCXX_QT)
    ODBCXX_SSTREAM* s=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_SSTREAM();
    if(b.getSize()>0) {
      s->write((ODBCXX_CHAR_TYPE*)b.getData(),b.getSize());
    }
    return s;
#else // ODBCXX_QT
    QBuffer* buf=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) QBuffer(b.copy());
    buf->open(IO_ReadOnly);
    return buf;
#endif
  }


} // namespace odbc


#endif
