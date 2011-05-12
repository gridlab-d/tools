// KLU DLL Export - Wrapper for KLU in GridLAB-D
//
// Copyright (c) 2011, Battelle Memorial Institute
// All Rights Reserved
//
// Author	Frank Tuffner, Pacific Northwest National Laboratory
// Created	May 2011
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
//   license at http://www.gnu.org/licenses/lgpl-2.1.html.
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
//   This code was implemented to facilitate the interface between
//   GridLAB-D and KLU. For usage please consult the GridLAB-D wiki at  
//   http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Powerflow_External_LU_Solver_Interface.
//
//   As a general rule, KLU should run about 30% faster than the standard
//   SuperLU solver embedded in GridLAB-D because it uses a method that 
//   assume sparsity and handles it better.  For details, see the
//   documentation of KLU itself.
//
//
// INSTALLATION
//
//   After building the release version, on 32-bit machines copy 
//   solver_klu_win32.dll to solver_klu.dll in the GridLAB-D folder that 
//   contains powerflow.dll.  On 64-bit machines copy solver_klu_x64.dll 
//   to solver_klu.dll in the folder that contains powerflow.dll.
// 