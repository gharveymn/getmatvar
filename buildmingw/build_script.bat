@echo off
cmake .. -G "MinGW Makefiles"
mingw32-make
mingw32-make install
del "../bin/*.dll.a"
del "../bin/*.pdb"