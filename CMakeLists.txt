CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(getmatvar)

MESSAGE(STATUS "Building with configuration CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")

	SET(OUT_NAME mexemulate)

	SET(CMAKE_CXX_STANDARD 11)

	if(MSVC OR (DEFINED USE_MSVC))
		MESSAGE(STATUS "Building with MSVC")
		ADD_DEFINITIONS(-DNO_MEX)
	else(MSVC OR (DEFINED USE_MSVC))
		MESSAGE(STATUS "Building with MinGW")
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -DNO_MEX")
	ENDIF(MSVC OR (DEFINED USE_MSVC))

	SET(PTHREADS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/include)

	# 32-bit or 64-bit mex
	IF(WIN32)
		IF((CMAKE_SIZEOF_VOID_P EQUAL 4) OR ("${BUILD_ENV}" STREQUAL "32BIT"))
			MESSAGE(STATUS "Compiling on 32 bit Windows")
			SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/win)
			SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/win/libdeflate.lib)

		ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			MESSAGE(STATUS "Compiling on 64 bit Windows")
			SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win)
			SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/win/libdeflate.lib)
		ENDIF()
	ELSE(WIN32)
		IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
			MESSAGE(STATUS "Compiling on 32 bit Linux")
		ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
			MESSAGE(STATUS "Compiling on 64 bit Linux")
		ENDIF()
	ENDIF(WIN32)

	INCLUDE_DIRECTORIES(${LIB_DEFLATE_INCLUDE_DIR} ${PTHREADS_INCLUDE_DIR})

	SET(SOURCE_FILES
			src/extlib/libdeflate/x64/win/libdeflate.h
			src/extlib/mman-win32/mman.c
			src/extlib/mman-win32/mman.h
			src/placeChunkedData.c
			src/navigate.c
			src/getSystemInfo.c
			src/getDataObjects.c
			src/headers/getDataObjects.h
			src/numberHelper.c
			src/ezq.c
			src/headers/ezq.h
			src/readMessage.c
			tests/mexemulate.c
			src/superblock.c
			src/createDataObjects.c
			src/init.c
			src/headers/init.h
			src/fillDataObjects.c
			src/headers/fillDataObjects.h
			src/utils.c src/headers/utils.h
			src/placeData.c
			src/headers/placeData.h
			src/cleanup.c
			src/headers/cleanup.h
			src/headers/placeChunkedData.h
			src/headers/readMessage.h
			src/headers/numberHelper.h
			src/headers/getSystemInfo.h
			src/headers/navigate.h
			src/headers/superblock.h src/mtezq.c src/headers/mtezq.h)

	ADD_EXECUTABLE(${OUT_NAME} ${SOURCE_FILES})

#	IF("${BUILD_ENV}" STREQUAL "32BIT")
#		set_target_properties(${OUT_NAME} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
#	ELSE("${BUILD_ENV}" STREQUAL "32BIT")
#		set_target_properties(${OUT_NAME} PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
#	ENDIF("${BUILD_ENV}" STREQUAL "32BIT")

	TARGET_LINK_LIBRARIES(${OUT_NAME} ${LIB_DEFLATE_LIBRARY})

ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")

	SET(INSTALL_OUTPUT_PATH "bin")
	SET(CMAKE_CL_64 TRUE)
	#set(CMAKE_VERBOSE_MAKEFILE ON)
	FIND_PATH(MATLAB_ROOT "bin/matlab.exe")
	SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})
	SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake) # add FindMatlab module

	ADD_DEFINITIONS(-DMATLAB_MEX_FILE) #define matlab macros
	ADD_DEFINITIONS(-DMX_COMPAT_32)

	FIND_PACKAGE(Matlab REQUIRED)

	IF(MATLAB_FOUND)
		MESSAGE(STATUS "MATLAB Found, MATLAB MEX will be compiled.")
		ADD_SUBDIRECTORY(src)
	ELSE(MATLAB_FOUND)
		MESSAGE("MATLAB not found... nothing will be built.")
	ENDIF(MATLAB_FOUND)

ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "NO_MEX")