This folder contains a simple example of extracting the admittance matrix from
the NR solver for an IEEE 4-node test system.  This includes the GLM example
for the extraction, as well as some sample MATLAB code to read in the
admittance matrix values.

Note that, as of January 8, 2016, this functionality only works with the
version of GridLAB-D located in ticket branch 730.  This functionality is
expected to be incorporated into the main GridLAB-D release in version 3.3 or
4.0, whichever is released first.

**** Included files ****
admittance_dump.txt - Sample output admittance dump from the GLM file
fun_GLD_Matrix_Output_Parse.m - MATLAB function to actually parse the text file
IEEE_4node_Admittance_Dump.glm - GridLAB-D model file of the IEEE 4-node
								 Wye-Wye, stepdown, unbalanced test system with
								 the appropriate settings to produce the
								 admittance dump.
Read_IEEE_4Node_Admittance.m - MATLAB script file to call the parsing function
                               and plot the results using MATLAB "spy"
README.txt - This file

**** Example ****
To output and examine the admittance matrix for the included IEEE 4-node system,
perform the following steps:

1) Run the IEEE_4node_Admittance_Dump.glm with an appropriate version of
   GridLAB-D.
2) Open the Read_IEEE_4Node_Admittance.m script file in MATLAB.  This is already
   set to read in the file produced by the IEEE_4node_Admittance_Dump.glm.
3) Execute the code, which will read in the admittance matrix and provide the
   sparse matrix view of it (MATLAB's spy command).
   
**** GLM Properties ****
Extracting the admittance matrix from GridLAB-D is done via the settings of
three powerflow module directives:

- NR_matrix_file
	This is the name of the text file output.  The name can only be 256
	characters long and MUST be specified.

- NR_matrix_output_interval
	This specifies how often the matrix output occurs.  The options are:
		NEVER    - The matrix is never dumped.  This is the default setting
		ONCE     - The matrix is output only for the first powerflow call and
				   first iteration.
		PER_CALL - The matrix is output only once per unique powerflow call,
		           on the first iteration of that solution.
		ALL      - The matrix is output for every single powerflow call and for
		           every iteration of that powerflow call.

- NR_matrix_output_references
	This specifies if the matrix location information should be included.  This
	provides a means for knowing which rows/columns of the admittance matrix
	correspond to which node object in the powerflow.  Options are:
		false - The information is not included in the output. Default setting.
		true  - Row and name information is output in the text file, but only
		        on the first iteration of each unique call to powerflow.

**** MATLAB files ****
Note that the Read_IEEE_4Node_Admittance.m script file reads in the location
information, but does not do anything with it.

Also note that the parsing function, fun_GLD_Matrix_Output_Parse.m *REQUIRES*
the NR_matrix_output_references flag to be set.  If it is not set, the output
file format will be slightly different and the function may fail.

MATLAB parsing function syntax (fun_GLD_Mastrix_Output_Parse.m):

Syntax:
[YMatrixGrid,LocationRows,LocationNames]=fun_GLD_Matrix_Output_Parse(filenamein)

Input:
- filenamein    = full path to the output text file

Outputs:
- YMatrixGrid   = full representation (not sparse) admittance matrix
- LocationRows  = start and stop row associated with each bus in YMatrixGrid
- LocationNames = bus names associated with the LocationRows
