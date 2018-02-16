CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(getmatvar)

SET(MEX_FILE_NAME getmatvar_)

INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})

SET(SOURCE_FILES
		${MATLAB_INCLUDE_DIR}/mex.h
		extlib/mman-win32/mman.c
		extlib/mman-win32/mman.h
		extlib/libdeflate/libdeflate.h
		navigate.c
		getSystemInfo.c
		getDataObjects.c
		headers/getDataObjects.h
		numberHelper.c
		ezq.c
		placeChunkedData.c
		mexSet.c
		readMessage.c
		headers/getmatvar_.h
		headers/ezq.h
		getmatvar_.c
		superblock.c
		createDataObjects.c
		init.c
		headers/init.h
		fillDataObjects.c
		headers/fillDataObjects.h
		utils.c headers/utils.h
		placeData.c
		headers/placeData.h
		cleanup.c
		headers/cleanup.h
		headers/placeChunkedData.h
		headers/readMessage.h
		headers/numberHelper.h
		headers/getSystemInfo.h
		headers/navigate.h
		headers/superblock.h
		headers/createDataObjects.h mtezq.c headers/mtezq.h)

ADD_LIBRARY(${MEX_FILE_NAME} SHARED ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/Matlabdef.def)

# 32-bit or 64-bit mex
IF(WIN32)
	IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE(STATUS "Compiling on 64 bit Windows")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/win/x64)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/win/x64/libdeflate.lib)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw64 PREFIX "")

	ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		MESSAGE(STATUS "Compiling on 32 bit Windows")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/win/x86)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/win/x86/libdeflate.lib)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw32 PREFIX "")
	ENDIF()
ELSE(WIN32)
	IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE(STATUS "Compiling on 64 bit Linux")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/unix)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/unix/libdeflate.a)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexa64 PREFIX "")
	ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		MESSAGE(STATUS "Compiling on 32 bit Linux")
		SET(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/unix)
		SET(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/unix/libdeflate.a)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexglx PREFIX "")
	ENDIF()
ENDIF(WIN32)

INCLUDE_DIRECTORIES(${LIB_DEFLATE_INCLUDE_DIR})
TARGET_LINK_LIBRARIES(${MEX_FILE_NAME} ${LIB_DEFLATE_LIBRARY})
TARGET_LINK_LIBRARIES(${MEX_FILE_NAME} ${MATLAB_MEX_LIBRARY} ${MATLAB_MX_LIBRARY})

INSTALL(TARGETS ${MEX_FILE_NAME} DESTINATION ${INSTALL_OUTPUT_PATH})
