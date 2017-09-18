cmake .. -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND"
mingw32-make
mingw32-make install
del "../bin/*.dll.a"