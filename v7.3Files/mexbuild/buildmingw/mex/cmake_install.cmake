# Install script for directory: C:/workspace/c/MatFile-Parsing/MatFile-Parsing-Win/v7.3Files/mexbuild/mex

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/workspace/c/MatFile-Parsing/MatFile-Parsing-Win/v7.3Files/mexbuild")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/../bin" TYPE STATIC_LIBRARY OPTIONAL FILES "C:/workspace/c/MatFile-Parsing/MatFile-Parsing-Win/v7.3Files/mexbuild/buildmingw/mex/libgetmatvar.dll.a")
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/../bin" TYPE SHARED_LIBRARY FILES "C:/workspace/c/MatFile-Parsing/MatFile-Parsing-Win/v7.3Files/mexbuild/buildmingw/mex/getmatvar.mexw64")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/../bin/getmatvar.mexw64" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/../bin/getmatvar.mexw64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "C:/msys64/mingw64/bin/strip.exe" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/../bin/getmatvar.mexw64")
    endif()
  endif()
endif()

