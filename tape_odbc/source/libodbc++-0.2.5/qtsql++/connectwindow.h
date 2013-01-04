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

#ifndef CONNECTWINDOW_H
#define CONNECTWINDOW_H

#include <qwidget.h>
#include <qdialog.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qpushbutton.h>

namespace odbc { class Connection; }


class ConnectWindow : public QDialog {
  Q_OBJECT

public:
  ConnectWindow(QWidget* parent =NULL, const char* name =NULL);
  virtual ~ConnectWindow();

  odbc::Connection* getConnection() const {
    return con;
  }


private slots:
  void tryConnect();

private:
  odbc::Connection* con;
  QLineEdit* userBox;
  QLineEdit* passwordBox;
  QComboBox* dataSources;

  void setup();
};



#endif
