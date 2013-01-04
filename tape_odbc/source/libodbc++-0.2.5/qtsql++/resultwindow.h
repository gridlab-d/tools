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

#ifndef RESULTWINDOW_H
#define RESULTWINDOW_H


#include <qdialog.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qsemimodal.h>

#include <odbc++/resultset.h>
#include <odbc++/statement.h>


class ResultWindow : public QDialog {
  Q_OBJECT

public:
  ResultWindow(odbc::Statement* stmt);
  virtual ~ResultWindow();

protected slots:
  void nextResultSet();

private:
  odbc::Statement* stmt_;
  odbc::ResultSet* rs_;

  QListView* list_;
  QPushButton* next_;

  void setup();
  void fetchResults();

};


class FetchInfo : public QSemiModal {
  Q_OBJECT

public:
  FetchInfo(QWidget* parent =NULL, const char* name =NULL);
  virtual ~FetchInfo();

  void setNumResults(unsigned int n);
  bool isCanceled() const {
    return canceled_;
  }
  
private:
  bool canceled_;
  unsigned int nr_;
  
  QLabel* l_;
  QPushButton* stop_;

private slots:
  void stopClicked();
};


#endif
