CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(MatFile_Parsing_Win)

SET(MEX_FILE_NAME getmatvar_)

SET(CMAKE_CXX_STANDARD 11)
INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})

SET(SOURCE_FILES
		${MATLAB_INCLUDE_DIR}/mex.h
		extlib/mman-win32/mman.c
		extlib/mman-win32/mman.h
		extlib/thpool/thpool.c
		extlib/thpool/thpool.h
		extlib/libdeflate/x64/libdeflate.h
		extlib/pthreads-win32/include/pthread.h
		fileHelper.c
		getSystemInfo.c
		mapping.c
		mapping.h
		numberHelper.c
		ezq.c
		chunkedData.c
		mexPointerSetters.c
		readMessage.c
		getmatvar_.h
		ezq.h
		getmatvar_.c)

ADD_LIBRARY(${MEX_FILE_NAME} SHARED ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/Matlabdef.def)

SET(PTHREADS_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/include)
SET(THPOOL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/thpool)

# 32-bit or 64-bit mex
IF(WIN32)
	IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE(STATUS "Compiling on 64 bit Windows")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/libdeflate.lib)
		SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x64/pthreadVC2.lib)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw64 PREFIX "")

	ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		MESSAGE(STATUS "Compiling on 32 bit Windows")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/libdeflate.lib)
		SET(PTHREADS_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/pthreads-win32/lib/x86/pthreadVC2.lib)
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
TARGET_LINK_LIBRARIES(${MEX_FILE_NAME} ${LIB_DEFLATE_LIBRARY} ${PTHREADS_LIBRARY})
TARGET_LINK_LIBRARIES(${MEX_FILE_NAME} ${MATLAB_MEX_LIBRARY} ${MATLAB_MX_LIBRARY})

INSTALL(TARGETS ${MEX_FILE_NAME} DESTINATION ${INSTALL_OUTPUT_PATH})
