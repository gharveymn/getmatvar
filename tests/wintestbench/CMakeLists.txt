CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
PROJECT(wintestbench)

SET(OUT_NAME memtest)

SET(CMAKE_CXX_STANDARD 11)

SET(SOURCE_FILES
		src/extlib/libdeflate/x64/libdeflate.h
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
		src/queue.c
		src/queue.h
		src/readMessage.c
		tests/memtest.c)

ADD_EXECUTABLE(${OUT_NAME} ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(${OUT_NAME} ${CMAKE_SOURCE_DIR}/src/extlib/libdeflate/x64/libdeflate.lib)