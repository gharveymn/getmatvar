cmake_minimum_required(VERSION 3.8)
project(MatFile_Parsing_WinJB)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   test.c)

add_executable(MatFile_Parsing_WinJB ${SOURCE_FILES})