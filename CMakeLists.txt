cmake_minimum_required(VERSION 3.7)
project(MatFile_Parsing_Win_Mex)

set(INSTALL_OUTPUT_PATH "bin")
set(CMAKE_CL_64 TRUE)
#set(CMAKE_VERBOSE_MAKEFILE ON)
find_path(MATLAB_ROOT "bin/matlab.exe")
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake) # add FindMatlab module

add_definitions(/DMATLAB_MEX_FILE) #define matlab macros
add_definitions(/DMX_COMPAT_32)

find_package(Matlab REQUIRED)

IF(MATLAB_FOUND)
    message(STATUS "MATLAB Found, MATLAB MEX will be compiled.")
    add_subdirectory(src)
ELSE(MATLAB_FOUND)
    MESSAGE("MATLAB not found...nothing will be built.")
ENDIF(MATLAB_FOUND)
