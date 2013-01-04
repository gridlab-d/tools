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

#include "mainwindow.h"
#include "connectwindow.h"
#include "resultwindow.h"

#include <qlayout.h>
#include <qsplitter.h>
#include <qstatusbar.h>
#include <qmessagebox.h>

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/statement.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>

#include <memory>

using namespace odbc;
using namespace std;

MainWindow::MainWindow()
  :QMainWindow(NULL,NULL),
   con_(NULL)
{
  this->setCaption("QTSQL++");
  fileMenu_=new QPopupMenu(this);

  connectId_=fileMenu_->insertItem("&Connect",this,SLOT(connect()));

  disconnectId_=fileMenu_->insertItem("&Disconnect",this,SLOT(disconnect()));
  fileMenu_->setItemEnabled(disconnectId_,false);

  fileMenu_->insertSeparator();

  fileMenu_->insertItem("E&xit",this,SLOT(fileExit()));

  menuBar()->insertItem("&File",fileMenu_);

  QWidget* c=new QWidget(this);
  this->setCentralWidget(c);
  QBoxLayout* vl=new QVBoxLayout(c);

  QBoxLayout* hl=new QHBoxLayout();

  sqlEdit_=new QMultiLineEdit(c);
  vl->addWidget(new QLabel(sqlEdit_,"&SQL:",c));
  vl->addWidget(sqlEdit_);
  vl->addLayout(hl);

  execute_=new QPushButton
    ("&Execute",c);
  hl->addWidget(execute_);

  QObject::connect(execute_,SIGNAL(clicked()),this,SLOT(executeQuery()));

  autoCommit_=new QPushButton("&Autocommit",c);
  autoCommit_->setToggleButton(true);
  hl->addWidget(autoCommit_);
  
  QObject::connect(autoCommit_,SIGNAL(clicked()),this,SLOT(autoCommit()));

  commit_=new QPushButton("&Commit",c);
  hl->addWidget(commit_);
  QObject::connect(commit_,SIGNAL(clicked()),this,SLOT(commit()));

  rollback_=new QPushButton("&Rollback",c);
  hl->addWidget(rollback_);
  QObject::connect(rollback_,SIGNAL(clicked()),this,SLOT(rollback()));

  // activate status bar
  (void)this->statusBar();

  this->_setupButtons();
}


MainWindow::~MainWindow()
{
}

void MainWindow::fileExit()
{
  if(con_!=NULL) {
    this->disconnect();
  }
  this->close();
}

void MainWindow::executeQuery()
{
  int l=sqlEdit_->numLines();
  QString sql;
  
  for(int i=0; i<l; i++) {
    if(i>0) {
      sql+="\n";
    }
    sql+=sqlEdit_->textLine(i);
  }
  
  try {
    std::auto_ptr<Statement> stmt(con_->createStatement());
    
    if(stmt->execute(sql)) {
      ResultWindow* rw=new ResultWindow(stmt.release());
    } else {
      statusBar()->message
	("Query executed, "+
	 QString::number(stmt->getUpdateCount())+
	 " rows affected",5000);
    }
    
  } catch(exception& e) {
    QMessageBox::warning
      (this,"Query failed",e.what(),
       QMessageBox::Ok,0,0);
  }
}

void MainWindow::connect()
{
  assert(con_==NULL);
  std::auto_ptr<ConnectWindow> cw(new ConnectWindow(this));

  if(cw->exec()==QDialog::Accepted) {
    fileMenu_->setItemEnabled(connectId_,false);
    fileMenu_->setItemEnabled(disconnectId_,true);
    con_=cw->getConnection();

    this->_setupButtons();

    this->statusBar()->message("Connected",3000);
  }

  this->setFocus();
}

void MainWindow::disconnect()
{
  assert(con_!=NULL);
  if(con_!=NULL) {
    delete con_;
    con_=NULL;
    statusBar()->message("Disconnected",3000);
    fileMenu_->setItemEnabled(disconnectId_,false);
    fileMenu_->setItemEnabled(connectId_,true);

    this->_setupButtons();
  }
}

void MainWindow::autoCommit()
{
  assert(con_!=NULL);
  bool b=autoCommit_->isOn();
  try {
    con_->setAutoCommit(b);
    autoCommit_->setOn(b);
    statusBar()->message(QString("Autocommit is now ")+(b?"on":"off"),3000);
  } catch(SQLException& e) {
    QMessageBox::warning
      (this,"Whoops",
       e.getMessage(),QMessageBox::Ok,0,0);
    autoCommit_->setOn(!b);
  }
}

void MainWindow::commit()
{
  assert(con_!=NULL);
  try {
    con_->commit();
    statusBar()->message("Transaction committed",3000);
  } catch(SQLException& e) {
    QMessageBox::warning
      (this,"Commit failed",
       e.getMessage(),QMessageBox::Ok,0,0);
  }
}

void MainWindow::rollback()
{
  assert(con_!=NULL);
  try {
    con_->rollback();
    statusBar()->message("Transaction rolled back",3000);
  } catch(SQLException& e) {
    QMessageBox::warning
      (this,"Rollback failed",
       e.getMessage(),QMessageBox::Ok,0,0);
  }
}

// private
// this assumes the buttons are created
void MainWindow::_setupButtons()
{

  if(con_!=NULL) {
    execute_->setEnabled(true);

    bool t=con_->getMetaData()->supportsTransactions();
    commit_->setEnabled(t);
    rollback_->setEnabled(t);

    bool ac;
    bool acEnabled=true;

    if(t) {
      
      try {
	ac=con_->getAutoCommit();
      } catch(SQLException& e) {
	ac=false;
	acEnabled=false;
      }

    } else {
      // no transaction support, we behave like autocommit was on
      ac=true;
      acEnabled=false;
    }

    autoCommit_->setEnabled(acEnabled);
    autoCommit_->setOn(ac);

  } else {
    execute_->setEnabled(false);
    commit_->setEnabled(false);
    rollback_->setEnabled(false);
    autoCommit_->setEnabled(false);
    autoCommit_->setOn(false);
  }

}
