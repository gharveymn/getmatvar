cmake_minimum_required(VERSION 3.7)
project(MatFile_Parsing_Win)

set(MEX_FILE_NAME getmatvar)

set(CMAKE_CXX_STANDARD 11)
INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})

set(SOURCE_FILES
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   chunkedData.c
	   getmatvar.c)
	   
add_library(${MEX_FILE_NAME} SHARED ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/Matlabdef.def)

# 32-bit or 64-bit mex
if(WIN32)
  if (CMAKE_CL_64)
      SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw64)
  else(CMAKE_CL_64)
      SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw32)
  endif(CMAKE_CL_64)
else(WIN32)
  if (CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexa64 PREFIX "")
  else(CMAKE_SIZEOF_VOID_P MATCHES "8")
      SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexglx PREFIX "")
  endif (CMAKE_SIZEOF_VOID_P MATCHES "8")
endif(WIN32)

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIRS})
	target_link_libraries(${MEX_FILE_NAME} ${ZLIB_LIBRARIES})
endif (ZLIB_FOUND)

install(TARGETS ${MEX_FILE_NAME} DESTINATION ../bin)
