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
#include <odbc++/errorhandler.h>

#include "datastream.h"

using namespace odbc;
using namespace std;

#if !defined(ODBCXX_QT)

DataStreamBuf::DataStreamBuf(ErrorHandler* eh, SQLHSTMT hstmt, int col,
                             int cType,DATASTATUS_TYPE& dataStatus)
  :errorHandler_(eh),
   hstmt_(hstmt),
   column_(col),
   cType_(cType),
   dataStatus_(dataStatus)
{
  switch(cType_) {
  case SQL_C_BINARY:
    bufferSize_=GETDATA_CHUNK_SIZE;
    break;

#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case SQL_C_TCHAR:
#else
  case SQL_C_CHAR:
#endif
    bufferSize_=GETDATA_CHUNK_SIZE+1;
    break;

  default:
    throw SQLException
      (ODBCXX_STRING_CONST("[libodbc++]: internal error, constructed stream for invalid type"), SQLException::scDEFSQLSTATE);
  }

  ODBCXX_CHAR_TYPE* gbuf=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[bufferSize_];
  this->setg(gbuf,gbuf+bufferSize_,gbuf+bufferSize_);
#ifdef THROWING_SHIT_IN_THE_CONSTRUCTOR_IS_A_BAD_IDEA_MAN
  // fetch the first chunk of data - otherwise we don't know whether it's
  // NULL or not
  try 
  {   this->underflow(); } 
  catch(...) 
  {
//    delete[] gbuf;
//	  ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__)[]	gbuf;
	  ODBCXX_DELETE_ARRPOINTER(gbuf, __FILE__, __LINE__);
	  this->setg(NULL,NULL,NULL);
    throw;
  }
#endif // THROWING_SHIT_IN_THE_CONSTRUCTOR_IS_A_BAD_IDEA_MAN
}

DataStreamBuf::~DataStreamBuf()
{
	ODBCXX_CHAR_TYPE* buffptr = this->eback();
	if(buffptr != NULL)
	{	//delete[] this->eback();}
		  ODBCXX_DELETE_ARRPOINTER(buffptr, __FILE__, __LINE__)
		  this->setg(NULL,NULL,NULL);
	}
}
//virtual
ODBCXX_STREAMBUF::int_type DataStreamBuf::underflow()
{
  if(gptr()<egptr()) {
    return (ODBCXX_CHAR_TYPE) *gptr();
  }

  //after the call, this is the number of bytes that were
  //available _before_ the call
  SQLLEN bytes;

  //the actual number of bytes that should end up in our buffer
  //we don't care about NULL termination
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  size_t bs=(cType_==SQL_C_TCHAR?bufferSize_-1:bufferSize_);
#else
  size_t bs=(cType_==SQL_C_CHAR?bufferSize_-1:bufferSize_);
#endif

  SQLRETURN r=SQLGetData(hstmt_,column_,
                         cType_,
                         (ODBCXX_SQLCHAR*)this->eback(),
                         bufferSize_*sizeof(ODBCXX_CHAR_TYPE),
                         &bytes);
  dataStatus_=bytes;

  errorHandler_->_checkStmtError(hstmt_,
                                 r,ODBCXX_STRING_CONST("Error fetching chunk of data"));

  if(r==ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    return EOF;
  }

  switch(bytes) {
  case SQL_NULL_DATA:
    return EOF;
    break;

  case SQL_NO_TOTAL:
    //The driver could not determine the size of the data
    //We can assume that the bytes read == our buffer size
    bytes=bs*sizeof(ODBCXX_CHAR_TYPE);
    break;

  case 0:
    return EOF;

  default:
    //as we're going to use bytes to set up our
    //pointers below, we adjust it to the number of bytes
    //we read
    bytes /= sizeof(ODBCXX_CHAR_TYPE);
    if(bytes>(SQLINTEGER)bs) {
      bytes=bs;
    }
    break;
  }

  this->setg(this->eback(), this->eback(), this->eback()+bytes);
  return (ODBCXX_CHAR_TYPE) *this->gptr();
}

#else // defined(ODBCXX_QT)


// QT QIODevice based implementation
// duplicates lots of code from above, but mixing the two up would become
// really ugly

DataStream::DataStream(ErrorHandler* eh, SQLHSTMT hstmt, int col,
                       int cType,SQLINTEGER& dataStatus)
  :errorHandler_(eh),
   hstmt_(hstmt),
   column_(col),
   cType_(cType),
   dataStatus_(dataStatus),
   bufferStart_(0),
   bufferEnd_(0),
   eof_(false)
{
  switch(cType_) {
  case SQL_C_BINARY:
    bufferSize_=GETDATA_CHUNK_SIZE;
    break;

#if defined(ODBCXX_HAVE_SQLUCODE_H)
  case SQL_C_TCHAR:
#else
  case SQL_C_CHAR:
#endif
    bufferSize_=GETDATA_CHUNK_SIZE+1;
    break;

  default:
    throw SQLException
      ("[libodbc++]: internal error, constructed stream for invalid type", SQLException::scDEFSQLSTATE);
  }

  buffer_=ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) ODBCXX_CHAR_TYPE[bufferSize_];
  try {
    this->_readStep(); // now we know whether we're NULL or not
  } catch(...) {
    // avoid leaking memory
//    delete[] buffer_;
	  ODBCXX_DELETE_ARRPOINTER(buffer_, __FILE__, __LINE__);

  }
}

DataStream::~DataStream()
{
//  delete[] buffer_;
	ODBCXX_DELETE_ARRPOINTER(buffer_, __FILE__, __LINE__);
}

// private
void DataStream::_readStep()
{
  SQLINTEGER bytes;

  // see above
#if defined(ODBCXX_HAVE_SQLUCODE_H)
  size_t bs=(cType_==SQL_C_TCHAR?bufferSize_-1:bufferSize_);
#else
  size_t bs=(cType_==SQL_C_CHAR?bufferSize_-1:bufferSize_);
#endif

  SQLRETURN r=SQLGetData(hstmt_,column_,
                         cType_,
                         (SQLPOINTER)buffer_,
                         bufferSize_*sizeof(ODBCXX_CHAR_TYPE),
                         &bytes);

  dataStatus_=bytes; // now a caller of ResultSet::wasNull() knows if this is NULL

  errorHandler_->_checkStmtError(hstmt_,
                                 r,"Error fetching chunk of data");

  if(r==ODBC3_C(SQL_NO_DATA,SQL_NO_DATA_FOUND)) {
    eof_=true;
    return;
  }

  switch(bytes) {
  case 0:
  case SQL_NULL_DATA:
    eof_=true;
    break;

  case SQL_NO_TOTAL:
    bytes=bs;
    break;

  default:
    if(bytes>(SQLINTEGER)bs) {
      bytes=bs;
    }
    break;
  }

  bufferStart_=0;
  bufferEnd_=(size_t)bytes;
}


DataStream::BlockRetType DataStream::readBlock(char* data, BlockLenType len)
{
  size_t bytesRead=0;
  while(!eof_ && bytesRead<len) {

    if(bufferEnd_-bufferStart_==0) {
      this->_readStep();
    }

    size_t b=bufferEnd_-bufferStart_;
    if(b>0) {

      if(b>len-bytesRead) {
        b=len-bytesRead;
      }

      memcpy((void*)data,(void*)&buffer_[bufferStart_],b*sizeof(ODBCXX_CHAR_TYPE));
      bufferStart_+=b;
      bytesRead+=b;
    }
  }

  return (int)bytesRead;
}

int DataStream::getch()
{
  if(eof_) {
    return -1;
  }

  if(bufferEnd_-bufferStart_>0) {
    return (ODBCXX_CHAR_TYPE)buffer_[bufferStart_++];
  }

  this->_readStep();
  if(eof_ || bufferEnd_-bufferStart_==0) {
    return -1;
  }
  return (ODBCXX_CHAR_TYPE)buffer_[bufferStart_++];
}


#endif
