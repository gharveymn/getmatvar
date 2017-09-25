@echo off

cmake .. -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND"  -DCMAKE_BUILD_TYPE="MEX"
mingw32-make
mingw32-make install
del "../bin/*.dll.a"