CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(MatFile_Parsing_Win_Mex)

MESSAGE(STATUS "Building with configuration CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")

	SET(OUT_NAME memtest)

	SET(CMAKE_CXX_STANDARD 11)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -DNO_MEX")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -DNO_MEX")

	SET(SOURCE_FILES
			src/extlib/libdeflate/x64/win/libdeflate.h
			src/extlib/mman-win32/mman.c
			src/extlib/mman-win32/mman.h
			src/extlib/thpool/thpool.c
			src/extlib/thpool/thpool.h
			src/extlib/param.h
			src/chunkedData.c
			src/fileHelper.c
			src/getSystemInfo.c
			src/mapping.c
			src/mapping.h
			src/numberHelper.c
			src/ezq.c
			src/ezq.h
			src/readMessage.c
			tests/memtest.c)

	ADD_EXECUTABLE(${OUT_NAME} ${SOURCE_FILES})

	TARGET_LINK_LIBRARIES(${OUT_NAME} ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win/libdeflate.lib)

	SET(OUT_NAME mexemulate)

	SET(SOURCE_FILES
			src/extlib/libdeflate/x64/win/libdeflate.h
			src/extlib/mman-win32/mman.c
			src/extlib/mman-win32/mman.h
			src/extlib/thpool/thpool.c
			src/extlib/thpool/thpool.h
			src/extlib/param.h
			src/chunkedData.c
			src/fileHelper.c
			src/getSystemInfo.c
			src/mapping.c
			src/mapping.h
			src/numberHelper.c
			src/ezq.c
			src/ezq.h
			src/readMessage.c
			tests/mexemulate.c)

	ADD_EXECUTABLE(${OUT_NAME} ${SOURCE_FILES})

	TARGET_LINK_LIBRARIES(${OUT_NAME} ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win/libdeflate.lib)

ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")

	SET(INSTALL_OUTPUT_PATH "bin")
	SET(CMAKE_CL_64 TRUE)
	#set(CMAKE_VERBOSE_MAKEFILE ON)
	FIND_PATH(MATLAB_ROOT "bin/matlab.exe")
	SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})
	SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake) # add FindMatlab module

	ADD_DEFINITIONS(/DMATLAB_MEX_FILE) #define matlab macros
	ADD_DEFINITIONS(/DMX_COMPAT_32)

	FIND_PACKAGE(Matlab REQUIRED)

	IF(MATLAB_FOUND)
		MESSAGE(STATUS "MATLAB Found, MATLAB MEX will be compiled.")
		ADD_SUBDIRECTORY(src)
	ELSE(MATLAB_FOUND)
		MESSAGE("MATLAB not found...nothing will be built.")
	ENDIF(MATLAB_FOUND)

ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")