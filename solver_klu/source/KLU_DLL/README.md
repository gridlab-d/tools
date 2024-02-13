# GridLAB-D KLU External Powerflow Solver

## Dependencies
### Installing KLU
The KLU External Solver requires an installation of KLU, now included in 
the SuiteSparse Library package. 

SuiteSparse is available in Ubuntu as `libsuitesparse-dev` and MinGW/MSYS2 as 
`mingw-w64-x86_64-suitesparse` 

#### Ubuntu installation
to install SuiteSparse on Ubuntu run the following command:
`sudo apt install libsuitesparse-dev`

#### MSYS2 installation
to install SuiteSparse on MSYS2 run the following command:
`pacman -S mingw-w64-x86_64-suitesparse`

### Other Dependencies
The build process for the KLU External Solver additionally requires the 
following:
1. CMake (v3.6 or later)
2. Build tools and compilers used for GridLAB-D


## Building KLU External Solver
1. Open KLU_DLL directory in terminal
2. Create build directory 
    * e.g., `mkdir build`
3. Move into directory 
    * e.g., `cd build`
3. Invoke CMake, and set installation directory to GridLAB-D install prefix 
    * e.g., `cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/gridlabd ../`
    * **note**: it is recommended for Windows builds to include `-G"CodeBlocks - Unix Makefiles"` in the `cmake` command, as MSVC compiler support is not guaranteed
4. Build and install the library 
    * e.g., `cmake --build . --target install` 
    * **note**: `sudo` may be required depending on installation target

The KLU External Solver will be compiled and installed in your GridLAB-D 
install directory.

If you are unable to directly install, step 4 can be replaced with 
`cmake --build .` and the produced `solver_KLU.dll` or `lib_solver_KLU.so` 
manually moved to GridLAB-D's install directory and placed in 
`<GridLAB-D root dir>/lib/gridlabd`

## GLM Formatting
To activate the library linkage, it is required to add the `lu_solver` 
tag in powerflow module section of your .GLM file, 
to indicate the external solver should be used.

Example powerflow module invocation:
``` cpp
module powerflow {
	lu_solver "KLU";
	solver_method NR;
}
```

## More Information
More extensive details related to the External LU Solver Interface can be found on the 
[Powerflow External LU Solver Interface](http://gridlab-d.shoutwiki.com/wiki/Powerflow_External_LU_Solver_Interface) 
page of the GridLAB-D Wiki.

