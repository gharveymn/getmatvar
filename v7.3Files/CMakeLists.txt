CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(MatFile_Parsing_WinJB)

SET(CMAKE_CXX_STANDARD 11)
SET(MEX "C:/Program Files/MATLAB/R2017a/extern/include")

INCLUDE_DIRECTORIES(${MEX})

MACRO(ADD_EXECUTABLE _name)
	# invoke built-in add_executable
	_ADD_EXECUTABLE(${ARGV})
	IF(TARGET ${_name})
		FIND_PACKAGE(ZLIB REQUIRED)
		IF(ZLIB_FOUND)
			INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
			TARGET_LINK_LIBRARIES(${_name} ${ZLIB_LIBRARIES})
		ENDIF(ZLIB_FOUND)
	ENDIF()
ENDMACRO()

SET(SOURCE_FILES
    extlib/mman-win32/mman.c
    extlib/mman-win32/mman.h
    fileHelper.c
    getPageSize.c
    mapping.c
    mapping.h
    numberHelper.c
    queue.c
    readMessage.c
    chunkedData.c
    userTest.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_userTest ${SOURCE_FILES})

SET(SOURCE_FILES
    extlib/mman-win32/mman.c
    extlib/mman-win32/mman.h
    fileHelper.c
    getPageSize.c
    mapping.c
    mapping.h
    numberHelper.c
    queue.c
    readMessage.c
    chunkedData.c
    chunkedTest.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_chunkedTest ${SOURCE_FILES})

SET(SOURCE_FILES
    extlib/mman-win32/mman.c
    extlib/mman-win32/mman.h
    fileHelper.c
    getPageSize.c
    mapping.c
    mapping.h
    numberHelper.c
    queue.c
    readMessage.c
    test.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_test ${SOURCE_FILES})

SET(SOURCE_FILES
    extlib/mman-win32/mman.c
    extlib/mman-win32/mman.h
    fileHelper.c
    getPageSize.c
    mapping.c
    mapping.h
    numberHelper.c
    queue.c
    readMessage.c
    test1.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_test1 ${SOURCE_FILES})

SET(SOURCE_FILES
    ${MEX}/mex.h
    extlib/mman-win32/mman.c
    extlib/mman-win32/mman.h
    fileHelper.c
    getPageSize.c
    mapping.c
    mapping.h
    numberHelper.c
    queue.c
    readMessage.c
    mexPointerSetters.c
    getmatvar.h
    getmatvar.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_mex ${SOURCE_FILES})