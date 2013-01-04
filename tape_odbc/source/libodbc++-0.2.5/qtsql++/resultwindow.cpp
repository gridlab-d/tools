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

#include "resultwindow.h"

#include <odbc++/resultsetmetadata.h>

#include <qlayout.h>
#include <qapplication.h>
#include <qpushbutton.h>

#include <memory>

using namespace odbc;


FetchInfo::FetchInfo(QWidget* parent,
		     const char* name)
  :QSemiModal(parent,name,true),
   canceled_(false),
   nr_(0)
{
  this->setCaption("Fetching results");
  QBoxLayout* vl=new QVBoxLayout(this);
  
  l_=new QLabel("0 rows fetched", this);
  l_->setAlignment(QLabel::AlignHCenter);
  vl->addWidget(l_,QLayout::AlignCenter);

  stop_=new QPushButton("&Stop",this);
  QObject::connect(stop_,SIGNAL(clicked()),this,SLOT(stopClicked()));
  vl->addWidget(stop_);
}


FetchInfo::~FetchInfo()
{
}


void FetchInfo::setNumResults(unsigned int n)
{
  nr_=n;
  l_->setText(QString::number(n)+" rows fetched");
  qApp->processEvents();
}

void FetchInfo::stopClicked()
{
  canceled_=true;
}


ResultWindow::ResultWindow(Statement* stmt)
  :QDialog(NULL,NULL),
   stmt_(stmt),
   rs_(stmt_->getResultSet())
{
  this->setCaption("Query results");
  this->setMaximumSize(800,600);

  list_=new QListView(this);
  list_->setAllColumnsShowFocus(true);
  
  QBoxLayout* vl=new QVBoxLayout(this);
  vl->addWidget(list_);

  QBoxLayout* hl=new QHBoxLayout();
  vl->addLayout(hl);

  next_=new QPushButton("&Next result set",this);
  QObject::connect(next_,SIGNAL(clicked()),this,SLOT(nextResultSet()));
  hl->addWidget(next_,0,QLayout::AlignLeft);

  QPushButton* b=new QPushButton("&Close",this);
  QObject::connect(b,SIGNAL(clicked()),this,SLOT(accept()));
  hl->addWidget(b,0,QLayout::AlignRight);
  

  this->setup();
  this->fetchResults();
  this->show();
}

ResultWindow::~ResultWindow()
{
  delete rs_;
  delete stmt_;
}


void ResultWindow::setup()
{
  ResultSetMetaData* md=rs_->getMetaData();
  int nc=md->getColumnCount();

  for(int i=1; i<=nc; i++) {
    list_->addColumn(md->getColumnName(i));
  }
}

void ResultWindow::fetchResults()
{
  ResultSetMetaData* md=rs_->getMetaData();
  int nc=md->getColumnCount();
  
  unsigned int cnt=0;
  bool canceled=false;

  list_->setUpdatesEnabled(false);

  std::auto_ptr<FetchInfo> fi(new FetchInfo(this));
  fi->show();


  while(!canceled && rs_->next()) {

    QListViewItem* item=new QListViewItem(list_);

    for(int i=1; i<=nc; i++) {
      QString t;
      bool unhandled=false;

      switch(md->getColumnType(i)) {
      case Types::BIT:
      case Types::TINYINT:
      case Types::SMALLINT:
      case Types::INTEGER:
	t=QString::number(rs_->getInt(i));
	break;

      case Types::NUMERIC:
      case Types::BIGINT:
      case Types::CHAR:
      case Types::VARCHAR:
	t=rs_->getString(i);
	break;

      case Types::DATE:
	t=rs_->getDate(i).toString();
	break;

      case Types::TIME:
	t=rs_->getTime(i).toString();
	break;

      case Types::TIMESTAMP:
	t=rs_->getTimestamp(i).toString();
	break;

      case Types::FLOAT:
      case Types::DOUBLE:
      case Types::REAL:
	t=QString::number(rs_->getDouble(i));
	break;

      default:
	t="?";
	unhandled=true;
	break;
      }

      if(!unhandled && rs_->wasNull()) {
	t="<NULL>";
      }

      item->setText(i-1,t);
    }

    if(cnt%20==0) {
      fi->setNumResults(cnt);
      canceled=fi->isCanceled();
    }
    cnt++;
  }

  list_->setUpdatesEnabled(true);
  
  delete rs_;
  rs_=NULL;
  
  if(!stmt_->getMoreResults()) {
    delete stmt_;
    stmt_=NULL;
    next_->setEnabled(false);
  }
}


void ResultWindow::nextResultSet()
{
  assert(stmt_!=NULL);
  
  ResultWindow* w=new ResultWindow(stmt_);
  stmt_=NULL;
}
