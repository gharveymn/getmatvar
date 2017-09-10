cmake_minimum_required(VERSION 3.7)
project(MatFile_Parsing_Win)

set(MEX_FILE_NAME getmatvar_)

set(CMAKE_CXX_STANDARD 11)
INCLUDE_DIRECTORIES(${MATLAB_INCLUDE_DIR})

set(SOURCE_FILES
	   ${MATLAB_INCLUDE_DIR}/mex.h
	   extlib/mman-win32/mman.c
	   extlib/mman-win32/mman.h
	   fileHelper.c
	   getPageSize.c
	   mapping.c
	   mapping.h
	   numberHelper.c
	   queue.c
	   chunkedData.c
	   mexPointerSetters.c
	   readMessage.c
	   getmatvar_.h
	   getmatvar_.c)

add_library(${MEX_FILE_NAME} SHARED ${SOURCE_FILES} ${CMAKE_SOURCE_DIR}/Matlabdef.def)

# 32-bit or 64-bit mex
if (WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE(STATUS "Compiling on 64 bit Windows")
		set(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64)
		set(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/libdeflate.lib)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw64)
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		MESSAGE(STATUS "Compiling on 32 bit Windows")
		set(LIB_DEFLATE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86)
		set(LIB_DEFLATE_LIBRARY ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x86/libdeflate.lib)
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexw32)
	endif()
else (WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE(STATUS "Compiling on 64 bit Linux")
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexa64 PREFIX "")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		MESSAGE(STATUS "Compiling on 32 bit Linux")
		SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES SUFFIX .mexglx PREFIX "")
	endif()
endif (WIN32)

include_directories(${LIB_DEFLATE_INCLUDE_DIR})
target_link_libraries(${MEX_FILE_NAME} ${LIB_DEFLATE_LIBRARY})

target_link_libraries(${MEX_FILE_NAME} ${MATLAB_MEX_LIBRARY} ${MATLAB_MX_LIBRARY})

SET_TARGET_PROPERTIES(${MEX_FILE_NAME} PROPERTIES PREFIX "")
install(TARGETS ${MEX_FILE_NAME} DESTINATION ${INSTALL_OUTPUT_PATH})
