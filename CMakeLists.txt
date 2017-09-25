CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(MatFile_Parsing_Win_Mex)

MESSAGE(STATUS "Building with configuration CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")

	SET(OUT_NAME mexemulate)

	SET(CMAKE_CXX_STANDARD 11)

	if(MSVC)
		ADD_DEFINITIONS(-DNO_MEX)
	else(MSVC)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -DNO_MEX")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -DNO_MEX")
	ENDIF(MSVC)

	SET(PTHREADS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/include)
	SET(THPOOL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/thpool)

	# 32-bit or 64-bit mex
	IF(WIN32)
		IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			MESSAGE(STATUS "Compiling on 64 bit Windows")
			SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win)
			SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win/libdeflate.lib)
			IF(MSVC)
				SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x64/pthreadVC2.lib)
			ELSE(MSVC)
				SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x64/libpthreadGC2.lib)
			ENDIF(MSVC)
			SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw64 PREFIX "")

		ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
			MESSAGE(STATUS "Compiling on 32 bit Windows")
			SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/win)
			SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/win/libdeflate.lib)
			IF(MSVC)
				SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x86/pthreadVC2.lib)
			ELSE(MSVC)
				SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x86/libpthreadGC2.lib)
			ENDIF(MSVC)
			SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw32 PREFIX "")
		ENDIF()
	ELSE(WIN32)
		IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			MESSAGE(STATUS "Compiling on 64 bit Linux")
			SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexa64 PREFIX "")
		ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
			MESSAGE(STATUS "Compiling on 32 bit Linux")
			SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexglx PREFIX "")
		ENDIF()
	ENDIF(WIN32)

	INCLUDE_DIRECTORIES(${LIB_DEFLATE_INCLUDE_DIR} ${PTHREADS_INCLUDE_DIR} ${THPOOL_INCLUDE_DIR})

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
	TARGET_LINK_LIBRARIES(${OUT_NAME} ${LIB_DEFLATE_LIBRARY} ${PTHREADS_LIBRARY})

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