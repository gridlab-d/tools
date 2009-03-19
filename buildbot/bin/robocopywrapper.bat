echo off
robocopy %*

if errorlevel 16 echo ***FATAL ERROR***  & exit /B 1
if errorlevel 8  echo **FAILED COPIES**  & exit /B 1
if errorlevel 4  echo *MISMATCHES*       & exit /B 1
if errorlevel 2  echo EXTRA FILES        & exit /B 1
if errorlevel 1  echo Copy successful    & exit /B 0
if errorlevel 0  echo --no change--      & exit /B 0
                                          