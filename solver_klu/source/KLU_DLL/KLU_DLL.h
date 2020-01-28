// KLU_DLL.h

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef struct {
	double *a_LU;
	double *rhs_LU;
	int *cols_LU;
	int *rows_LU;
} NR_SOLVER_VARS;

typedef struct {
	klu_common *CommonVal;
	klu_symbolic *SymbolicVal;
	klu_numeric *NumericVal;
	bool AdmittanceChange;
} KLU_STRUCT;

//Initialization function
extern "C" EXPORT void *LU_init(void *ext_array);

// Allocation function
extern "C" EXPORT void LU_alloc(void *ext_array, unsigned int rowcount, unsigned int colcount, bool admittance_change);

// Solver function
extern "C" EXPORT int LU_solve(void *ext_array, NR_SOLVER_VARS *system_info_vars, unsigned int rowcount, unsigned int colcount);

// Destructive function
extern "C" EXPORT void LU_destroy(void *ext_array, bool new_iteration);

