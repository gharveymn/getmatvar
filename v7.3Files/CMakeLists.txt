CMAKE_MINIMUM_REQUIRED(VERSION 3.7)
PROJECT(MatFile_Parsing_WinJB)

SET(CMAKE_CXX_STANDARD 11)
SET(MEX "C:/Program Files/MATLAB/R2017a/extern/include")
SET(MATLAB_LIBS "C:/Program Files/MATLAB/R2017a/extern/lib/win64/mingw64")

LINK_DIRECTORIES(${MATLAB_LIBS})
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

	TARGET_LINK_LIBRARIES(${_name} libmx)
ENDMACRO()

SET(SOURCE_FILES
    src/extlib/mman-win32/mman.c
    src/extlib/mman-win32/mman.h
    src/fileHelper.c
    src/getPageSize.c
    src/mapping.c
    src/mapping.h
    src/numberHelper.c
    src/queue.c
    src/readMessage.c
    src/chunkedData.c
    tests/userTest.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_userTest ${SOURCE_FILES})

SET(SOURCE_FILES
    src/extlib/mman-win32/mman.c
    src/extlib/mman-win32/mman.h
    src/fileHelper.c
    src/getPageSize.c
    src/mapping.c
    src/mapping.h
    src/numberHelper.c
    src/queue.c
    src/readMessage.c
    src/chunkedData.c
    tests/chunkedTest.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_chunkedTest ${SOURCE_FILES})

SET(SOURCE_FILES
    src/extlib/mman-win32/mman.c
    src/extlib/mman-win32/mman.h
    src/fileHelper.c
    src/getPageSize.c
    src/mapping.c
    src/mapping.h
    src/numberHelper.c
    src/queue.c
    src/readMessage.c
    tests/test.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_test ${SOURCE_FILES})

SET(SOURCE_FILES
    src/extlib/mman-win32/mman.c
    src/extlib/mman-win32/mman.h
    src/fileHelper.c
    src/getPageSize.c
    src/mapping.c
    src/mapping.h
    src/numberHelper.c
    src/queue.c
    src/readMessage.c
    tests/test1.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_test1 ${SOURCE_FILES})

SET(SOURCE_FILES
    ${MEX}/mex.h
    src/extlib/mman-win32/mman.c
    src/extlib/mman-win32/mman.h
    src/fileHelper.c
    src/getPageSize.c
    src/mapping.c
    src/mapping.h
    src/numberHelper.c
    src/queue.c
    src/readMessage.c
    src/mexPointerSetters.c
    src/getMatVar.h
    src/getMatVar.c)

ADD_EXECUTABLE(MatFile_Parsing_WinJB_mex ${SOURCE_FILES})