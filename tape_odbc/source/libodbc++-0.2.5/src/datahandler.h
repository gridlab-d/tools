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

#ifndef __DATAHANDLER_H
#define __DATAHANDLER_H

#if defined(ODBCXX_HAVE_ISTREAM)
# include <istream>
#else
# include <iostream>
#endif

#include <odbc++/types.h>

#if defined(ODBCXX_QT)
# include <qiodevice.h>
#endif

namespace odbc {

  class ODBCXX_EXPORT DataHandler {
    friend class ResultSet;
    friend class PreparedStatement;
    friend class Rowset;

  private:

    //actual buffer size is rows_ * bufferSize_
    unsigned int& currentRow_;
    size_t rows_;
    char* buffer_;
    size_t bufferSize_;
    DATASTATUS_TYPE* dataStatus_;
    bool isStreamed_;
    ODBCXX_STREAM* stream_;
    bool ownStream_;

    int sqlType_;
    int cType_;
    int precision_;
    int scale_;
    bool use3_;


    void setupBuffer(size_t s);

    //this returns a ptr to the current row's storage
    char* data() {
      return &buffer_[bufferSize_*currentRow_];
    }

    void resetStream() {
      if(isStreamed_) 
	  {
		if(ownStream_) 
		{
			ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) stream_;
			ownStream_=false;
		}
		stream_=NULL;
#ifdef CACHE_STREAMED_DATA
		// Clear any buffer
		setupBuffer(0);
#endif // CACHE_STREAMED_DATA
      } 
	  else {assert(stream_==NULL);     }
    }

    //same, const version
    const char* data() const {
      return &buffer_[bufferSize_*currentRow_];
    }

    void setDataStatus(SQLINTEGER i) {
      dataStatus_[currentRow_]=i;
    }

    SQLINTEGER getDataStatus() const {
      return dataStatus_[currentRow_];
    }

  public:
    DataHandler(unsigned int& cr, size_t rows,
		int sqlType, int precision, int scale, bool use3);

    ~DataHandler() {
      this->resetStream();
      this->setupBuffer(0);
      ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__)[] dataStatus_;
    }

    int getSQLType() const {
      return sqlType_;
    }

    bool getBoolean() const;
    signed char getByte() const;
    ODBCXX_BYTES getBytes() const;
    short getShort() const;
    int getInt() const;
    Long getLong() const;
    float getFloat() const;
    double getDouble() const;
#ifdef ODBCXX_HAVE_STRUCT_GUID
    Guid getGuid() const;
#endif

    Date getDate() const;
    Time getTime() const;
    Timestamp getTimestamp() const;
    ODBCXX_STRING getString() const;

    ODBCXX_STREAM* getStream() const;

    void setNull() {
      this->resetStream();
      this->setDataStatus(SQL_NULL_DATA);
    }

    bool isNull() const {
      return this->getDataStatus()==SQL_NULL_DATA;
    }

    void setBoolean(bool b);
    void setByte(signed char b);
    void setBytes(const ODBCXX_BYTES& b);
    void setShort(short s);
    void setInt(int i);
    void setLong(Long l);
    void setFloat(float f);
    void setDouble(double d);
    void setDate(const Date& d);
    void setTime(const Time& t);
    void setTimestamp(const Timestamp& ts);
    void setString(const ODBCXX_STRING& s);
#ifdef ODBCXX_HAVE_STRUCT_GUID
    void setGuid(const Guid& g);
#endif

    //this is called by ResultSet for a fetch - does not touch the data status
    void setStream(ODBCXX_STREAM* s);

    //this is called for a set - sets the data status to
    //data-at-exec(len)
    void setStream(ODBCXX_STREAM* s, int len, bool ownstream = false);

    //this is called when the rowset moves to another row
    void rowChanged() {
      this->resetStream();
    }

    //this is called after an insert/update has been done
    //using the rowset
    void afterUpdate() {
      this->resetStream();
    }

    static int defaultPrecisionFor(int sqlType) {
      int prec=0;
      switch(sqlType) {
      case Types::CHAR:
      case Types::VARCHAR:
      case Types::BINARY:
      case Types::VARBINARY:
	prec=255;
	break;

      case Types::TIMESTAMP:
	prec=19;
	break;
      }

      return prec;
    }

  };

  class ODBCXX_EXPORT Rowset {
  private:
    typedef std::vector<DataHandler*> DataHandlerList;

    DataHandlerList dataHandlers_;

    size_t rows_;

    unsigned int currentRow_;
    bool use3_;

    DataHandler* _createColumn(int sqlType, int precision, int scale) {
      return ODBCXX_OPERATOR_NEW_DEBUG(__FILE__, __LINE__) DataHandler
	(currentRow_,rows_,sqlType,precision,scale,use3_);
    }

  public:
    Rowset(size_t rows, bool use3)
      :rows_(rows),
       currentRow_(0),
       use3_(use3) {
      assert(rows_>0);
    }

    ~Rowset() {
      while(!dataHandlers_.empty()) {
	DataHandlerList::iterator i=dataHandlers_.begin();
	ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) *i;
	dataHandlers_.erase(i);
      }
    }

    void addColumn(int sqlType, int precision, int scale) {
      dataHandlers_.insert(dataHandlers_.end(),
			   this->_createColumn(sqlType,precision,scale));
    }

    void replaceColumn(int idx, int sqlType, int precision, int scale) {
      assert(idx>0 && idx<=(int)dataHandlers_.size());

      DataHandler* h=this->_createColumn(sqlType,precision,scale);
      ODBCXX_OPERATOR_DELETE_DEBUG(__FILE__, __LINE__) dataHandlers_[idx-1];
      dataHandlers_[idx-1]=h;
    }

    unsigned int getCurrentRow() const {
      return currentRow_;
    }

    void setCurrentRow(unsigned int rowNumber) {

      assert(rowNumber<rows_);

      currentRow_=rowNumber;
      for(DataHandlerList::iterator i=dataHandlers_.begin();
	  i!=dataHandlers_.end(); i++) {
	(*i)->rowChanged();
      }
    }

    DataHandler* getColumn(unsigned int idx) {
      assert(idx>0 && idx<=dataHandlers_.size());
      return dataHandlers_[idx-1];
    }


    size_t getRows() const {
      return rows_;
    }

    size_t getColumns() const {
      return dataHandlers_.size();
    }

    void afterUpdate() {
      for(DataHandlerList::iterator i=dataHandlers_.begin();
	  i!=dataHandlers_.end(); i++) {
	(*i)->afterUpdate();
      }
    }
  };
} // namespace odbc

#endif // __DATAHANDLER_H
