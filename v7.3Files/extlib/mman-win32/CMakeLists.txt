project(mman-win32 C)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES mman.c test.c)

add_executable(mmantest ${SOURCE_FILES})


