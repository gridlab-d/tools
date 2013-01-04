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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qwidget.h>
#include <qmainwindow.h>
#include <qmenubar.h>
#include <qpushbutton.h>
#include <qpopupmenu.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qmultilineedit.h>

namespace odbc { class Connection; }

class QSplitter;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow();
  virtual ~MainWindow();

public slots:
  void fileExit();
  void connect();
  void disconnect();
  void executeQuery();
  void autoCommit();
  void commit();
  void rollback();

private:
  QMultiLineEdit* sqlEdit_;
  QPopupMenu* fileMenu_;
  QPushButton* autoCommit_;
  QPushButton* execute_;
  QPushButton* commit_;
  QPushButton* rollback_;
  

  int connectId_;
  int disconnectId_;

  odbc::Connection* con_;

  void _setupButtons();
};

#endif
