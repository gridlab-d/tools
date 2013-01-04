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

#ifndef __DATASTREAM_H
#define __DATASTREAM_H

#include <odbc++/types.h>

#if defined(ODBCXX_HAVE_ISO_CXXLIB)
# include <istream>
# include <streambuf>
#else
# include <iostream>
# include <streambuf.h>
#endif

#if defined(ODBCXX_QT)
# include <qiodevice.h>
#endif

namespace odbc {

#if !defined(ODBCXX_QT)


   class DataStreamBuf : public ODBCXX_STREAMBUF {
    friend class DataStreamBase;
    friend class DataStream;
  private:
    ErrorHandler* errorHandler_;
    SQLHSTMT hstmt_;
    int column_;
    int cType_;
    DATASTATUS_TYPE& dataStatus_;
    size_t bufferSize_;

    virtual int_type underflow();

    virtual int_type overflow(int) {
      return EOF;
    }

    virtual int sync() {
      //what can we do?
      return 0;
    }

    virtual
#if !defined(ODBCXX_HAVE_ISO_CXXLIB)
    int
#else
    std::streamsize
#endif
    showmanyc() {
      if(this->gptr() < this->egptr()) {
	return this->egptr() - this->gptr();
      }
      return 0;
    }

    DataStreamBuf(ErrorHandler* eh, SQLHSTMT hstmt, int col, int cType,
		  DATASTATUS_TYPE& dataStatus);
    virtual ~DataStreamBuf();
  };

#if !defined(ODBCXX_HAVE_ISO_CXXLIB)

  class DataStreamBase : public virtual ios {
  private:
    DataStreamBuf buf_;

  protected:
    DataStreamBase(ErrorHandler* eh, SQLHSTMT hstmt, int column,
		   int cType,DATASTATUS_TYPE& ds)
      :buf_(eh,hstmt,column,cType,ds) {}

    virtual ~DataStreamBase() {}
    virtual DataStreamBuf* rdbuf() {
      return &buf_;
    }
  };

  class DataStream : public DataStreamBase, public ODBCXX_STREAM
#else
  class DataStream : public ODBCXX_STREAM
#endif
  {
    friend class ResultSet;
    friend class PreparedStatement;
    friend class Rowset;
  private:
    DataStream(ErrorHandler* eh, SQLHSTMT hstmt, int column, int cType,
	       DATASTATUS_TYPE& ds)
      :
#if !defined(ODBCXX_HAVE_ISO_CXXLIB)
      DataStreamBase(eh,hstmt,column,cType,ds),ODBCXX_STREAM(this->rdbuf())
#else
# if !defined(_MSC_VER)
      ODBCXX_STREAM(ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) odbc::DataStreamBuf(eh,hstmt,column,cType,ds))
# else
      // Some bug in MSC makes it fail to realise that std::istream
      // is inherited by this class
      std::basic_istream<ODBCXX_CHAR_TYPE, std::char_traits<ODBCXX_CHAR_TYPE> >
    (ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) odbc::DataStreamBuf(eh,hstmt,column,cType,ds))
# endif // _MSC_VER
#endif
    {
#ifndef THROWING_SHIT_IN_THE_CONSTRUCTOR_IS_A_BAD_IDEA_MAN
  // fetch the first chunk of data - otherwise we don't know whether it's
  // NULL or not
		odbc::DataStreamBuf* dsbuff = dynamic_cast<odbc::DataStreamBuf*>(rdbuf()); 
		if(dsbuff != NULL)
		{	dsbuff->underflow();}
#endif // THROWING_SHIT_IN_THE_CONSTRUCTOR_IS_A_BAD_IDEA_MAN
	  }
    ~DataStream() {
#if defined(ODBCXX_HAVE_ISO_CXXLIB)
      ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) this->rdbuf();
#endif
    }
  };

#else // defined(ODBCXX_QT)


  class DataStream : public QIODevice {
  public:

# if QT_VERSION >= 300
    typedef QIODevice::Offset SizeType;
    typedef Q_LONG BlockRetType;
    typedef Q_ULONG BlockLenType;
# else
    typedef uint SizeType;
    typedef int BlockRetType;
    typedef uint BlockLenType;
# endif

    DataStream(ErrorHandler* eh, SQLHSTMT hstmt, int column, int cType,
	       SQLINTEGER& ds);
    virtual ~DataStream();

    virtual bool open(int) {
      return false;
    }

    virtual void close() {}
    virtual void flush() {}

    virtual SizeType size() const {
      return 0;
    }

    virtual BlockRetType readBlock(char* data, BlockLenType len);

    // not writable
    virtual BlockRetType writeBlock(const char* data, BlockLenType len) {
      return -1;
    }

    virtual int putch(int) {
      return -1;
    }

    virtual int ungetch(int) {
      return -1;
    }

    virtual int getch();

  private:
    ErrorHandler* errorHandler_;
    SQLHSTMT hstmt_;
    int column_;
    int cType_;
    SQLINTEGER& dataStatus_;
    size_t bufferStart_;
    size_t bufferEnd_;
    bool eof_;
    size_t bufferSize_;
    char* buffer_;

    void _readStep();
  };
#endif
} // namespace odbc

#endif //__DATASTREAM_H
