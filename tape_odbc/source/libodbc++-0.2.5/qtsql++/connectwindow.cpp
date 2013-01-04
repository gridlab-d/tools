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

#include "connectwindow.h"
#include <iostream>

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>

#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmessagebox.h>

using namespace odbc;

ConnectWindow::ConnectWindow(QWidget* parent,
			     const char* name)
  :QDialog(parent,name,true),
   con(NULL)
{
  this->setCaption("Connect to datasource");

  QVBoxLayout* vl=new QVBoxLayout(this);
  QGridLayout* gl=new QGridLayout(3,2);
  QHBoxLayout* hl=new QHBoxLayout();

  vl->addLayout(gl);
  vl->addLayout(hl);


  dataSources=new QComboBox(false,this);
  gl->addWidget(dataSources,0,1);

  
  userBox=new QLineEdit(this);
  gl->addWidget(userBox,1,1);

  passwordBox=new QLineEdit(this);
  passwordBox->setEchoMode(QLineEdit::Password);
  gl->addWidget(passwordBox,2,1);

  gl->addWidget(new QLabel(dataSources,"&Datasource:",this),0,0);
  gl->addWidget(new QLabel(userBox,"&Username:",this),1,0);
  gl->addWidget(new QLabel(passwordBox,"&Password:",this),2,0);
  
  QPushButton* ok=new QPushButton("Connect",this);
  ok->setDefault(true);
  QPushButton* cancel=new QPushButton("Cancel",this);

  hl->addWidget(ok);
  hl->addWidget(cancel);

  connect(ok,SIGNAL(clicked()),this,SLOT(tryConnect()));
  connect(cancel,SIGNAL(clicked()),this,SLOT(reject()));

  QSize sh=this->sizeHint();
  this->setMaximumHeight(sh.height());

  this->setup();
}

void ConnectWindow::setup()
{
  DataSourceList* l=DriverManager::getDataSources();
  for(DataSourceList::iterator i=l->begin();
      i!=l->end(); i++) {
    DataSource* ds=*i;
    dataSources->insertItem(ds->getName());
  }
  delete l;

  dataSources->setFocus();
}


ConnectWindow::~ConnectWindow()
{
}

void ConnectWindow::tryConnect()
{
  try {
    con=DriverManager::getConnection
      (dataSources->currentText(),
       userBox->text(),
       passwordBox->text());
    
    this->accept();

  } catch(SQLException& e) {
    QMessageBox::warning(this,"Connect failed",
			 e.getMessage(),
			 QMessageBox::Ok,0,0);
    con=NULL;
  }
}
