// tape_odbc Export - IO Extension for GridLAB-D
//
// Copyright (c) 2012, Battelle Memorial Institute
// All Rights Reserved
//
// Author	Matt Hauer, Pacific Northwest National Laboratory
// Created	December 2012
//
// Pacific Northwest National Laboratory is operated by Battelle 
// Memorial Institute for the US Department of Energy under 
// Contract No. DE-AC05-76RL01830.
//
//
// LICENSE
//
//   This software is released under the Lesser General Public License
//   (LGPL) Version 2.1.  You can read the full text of the LGPL
//   license at http://www.gnu.org/licenses/lgpl-2.1.html.  A copy of
//   the LGPL license is additionally included in source/libodbc++-0.2.5/COPYING.
//
//
// US GOVERNMENT RIGHTS
//
//   The Software was produced by Battelle under Contract No. 
//   DE-AC05-76RL01830 with the Department of Energy.  The U.S. 
//   Government is granted for itself and others acting on its 
//   behalf a nonexclusive, paid-up, irrevocable worldwide license 
//   in this data to reproduce, prepare derivative works, distribute 
//   copies to the public, perform publicly and display publicly, 
//   and to permit others to do so.  The specific term of the license 
//   can be identified by inquiry made to Battelle or DOE.  Neither 
//   the United States nor the United States Department of Energy, 
//   nor any of their employees, makes any warranty, express or implied, 
//   or assumes any legal liability or responsibility for the accuracy, 
//   completeness or usefulness of any data, apparatus, product or 
//   process disclosed, or represents that its use would not infringe 
//   privately owned rights.  
//
//
// DESCRIPTION
//
//   This code was implemented to allow GridLAB-D to read and write data
//   to ODBC-compatible data sources.  For usage information, please review
//   http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Tape_Module_Guide#ODBC
//
//   The tape_odbc module will communicate with three tables with a specific
//   schema, using the class interface and the output structure of the tape
//   objects.  These tables must be OS-recognized ODBC data sources.  See
//   the documentation for the specific schema and table names.
//
// INSTALLATION
//
//   First, build a copy of libodbc++ for the local machine.
//   A copy of the build instructions can be found at
//   http://libodbcxx.sourceforge.net/libodbc++/INSTALL/book1.html
//   The non-QT DLL and LIB are required.
//   Second, place the output binaries in the tape_odbc/source directory.
//   If compiling a debug version of GridLAB-D, use the debug libodbc++
//   binaries.  If compiling a release version of GridLAB-D, use the prod
//   libodbc++ binaries.
//   Compile tape_odbc using the provided Visual Studio 2005 solution.  The
//   "23" project will use the branch/2.3 headers, and the "trunk" project will
//   use the trunk headers.
//   Third, copy tape_odbc.dll and libodbc++.dllto the directory with tape.dll,
//   such as VS2005/Win32/Debug.
//
//   
// KNOWN ISSUES
//
//   There is no 64-bit Windows version of libodbc++ available at this time.
//   There is no Linux makefile for tape_odbc at this time.
//
