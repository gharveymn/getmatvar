cmake_minimum_required(VERSION 3.7)
project(MatFile_Parsing_WinJB)

set(CMAKE_CXX_STANDARD 11)
set(MEX "C:/Program Files/MATLAB/R2017a/extern/include")

include_directories(${MEX})

macro (add_executable _name)
	# invoke built-in add_executable
	_add_executable(${ARGV})
	if (TARGET ${_name})
		find_package(ZLIB REQUIRED)
		if (ZLIB_FOUND)
			include_directories(${ZLIB_INCLUDE_DIRS})
			target_link_libraries(${_name} ${ZLIB_LIBRARIES})
		endif (ZLIB_FOUND)
	endif()
endmacro()

set(SOURCE_FILES
	   ${MEX}/mex.h
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   chunkedData.c
	   userTest.c)

add_executable(MatFile_Parsing_WinJB_userTest ${SOURCE_FILES})

set(SOURCE_FILES
	   ${MEX}/mex.h
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   chunkedData.c
	   chunkedTest.c)

add_executable(MatFile_Parsing_WinJB_chunkedTest ${SOURCE_FILES})

set(SOURCE_FILES
	   ${MEX}/mex.h
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   test.c)

add_executable(MatFile_Parsing_WinJB_test ${SOURCE_FILES})

set(SOURCE_FILES
	   ${MEX}/mex.h
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   test1.c)

add_executable(MatFile_Parsing_WinJB_test1 ${SOURCE_FILES})