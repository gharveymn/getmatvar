@echo off

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OS=32BIT || set OS=64BIT
if %OS%==32BIT cmake .. -DCMAKE_GENERATOR_PLATFORM=x86 -DCMAKE_BUILD_TYPE="NO_MEX"
if %OS%==64BIT cmake .. -DCMAKE_GENERATOR_PLATFORM=x64 -DCMAKE_BUILD_TYPE="NO_MEX"