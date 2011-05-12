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

#include "klu.h"
#include "KLU_DLL.h"

// Initialization function
// Sets Common property (options)
void *LU_init(void *ext_array)
{
	// Recasting variable
	KLU_STRUCT *KLUValues;

	// See if the external array is linked yet
	if (ext_array==NULL)	//Nope
	{
		// Create an array
		KLUValues = (KLU_STRUCT*)malloc(sizeof(KLU_STRUCT));

		// Make sure it worked
		if (KLUValues==NULL)
		{
			//Needs to be caught externally
			return NULL;
		}

		// Store the value
		ext_array = (void *)(KLUValues);

		// Zero the entries
		KLUValues->CommonVal = NULL;
		KLUValues->SymbolicVal = NULL;
		KLUValues->NumericVal = NULL;

		// Flag as none initially
		KLUValues->AdmittanceChange = false;	
	}

	// Already linked, link the variable to it
	else	
	{
		// Link the structure up
		KLUValues = (KLU_STRUCT*)ext_array;
	}

	// Determine if the common property is allocated yet
	if (KLUValues->CommonVal==NULL)
	{
		// Allocate it
		KLUValues->CommonVal = (klu_common *)malloc(sizeof(klu_common));

		// Make sure it worked
		if (KLUValues->CommonVal==NULL)
		{
			// Needs to be caught externally
			return NULL;
		}
	}

	// Set the defaults
	klu_defaults(KLUValues->CommonVal);

	return ext_array;
}

// Allocation function
// Basically useless for KLU since it needs real matrices already populated, so this routine doesn't do much
void LU_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change)
{
	// Recasting variable
	KLU_STRUCT *KLUValues;

	// Link the structure up
	KLUValues = (KLU_STRUCT*)ext_array;

	// Capture the admittance change - need it later
	KLUValues->AdmittanceChange = admittance_change;
}

// Solution function
// allocates an initial symbolic array and changes it as necessary
// allocates numeric array each pass
// Performs the analysis portion and outputs result
int LU_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount)
{
	// Recasting variable
	KLU_STRUCT *KLUValues;

	// Link the structure up
	KLUValues = (KLU_STRUCT*)ext_array;

	// See if the admittance has changed
	if (KLUValues->AdmittanceChange)
	{
		// First run - flagged as an admittance change by default
		if (KLUValues->SymbolicVal==NULL)	
		{
			KLUValues->SymbolicVal = klu_analyze (rowcount, system_info_vars->cols_LU, system_info_vars->rows_LU, KLUValues->CommonVal);	//Theoretically only needs to be done once - no admittance change means the overall structure is preserved
		}

		// Not first run
		else 
		{
			// Remove the old
			klu_free_symbolic (&(KLUValues->SymbolicVal), KLUValues->CommonVal);

			// Allocate
			KLUValues->SymbolicVal = klu_analyze (rowcount, system_info_vars->cols_LU, system_info_vars->rows_LU, KLUValues->CommonVal);	//Theoretically only needs to be done once
		}
		// Default else - if not an admittance change, leave it alone (structure didn't move)
	}

	// Create numeric one no matter what
	KLUValues->NumericVal = klu_factor(system_info_vars->cols_LU,system_info_vars->rows_LU,system_info_vars->a_LU,KLUValues->SymbolicVal,KLUValues->CommonVal);

	// Solve the matrix
	klu_solve(KLUValues->SymbolicVal,KLUValues->NumericVal, rowcount, colcount, system_info_vars->rhs_LU,KLUValues->CommonVal);

	// For KLU - 1 = singular matrix (if failure turned off), positive values = warnings, negative = bad, -2 = Out of Memory, -3 = Invalid matrix (or singular if error), -4 = Too Large
	return KLUValues->CommonVal->status;
}

// Destruction function
// Frees up numeric array
// New iteration isn't needed here - numeric gets redone EVERY iteration, so we don't care if we were successful or not
void LU_destroy(void *ext_array, bool new_iteration)
{
	// Recasting variable
	KLU_STRUCT *KLUValues;

	// Link the structure up
	KLUValues = (KLU_STRUCT*)ext_array;

	// KLU destructive commands
	klu_free_numeric(&(KLUValues->NumericVal),KLUValues->CommonVal);
}
