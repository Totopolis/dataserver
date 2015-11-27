cmake_minimum_required(VERSION 2.6)
project(dataserver C CXX)

# support C++11 with Clang
set(CMAKE_CXX_FLAGS "-Wall -std=c++11")

set(SDL_VERSION_MAJOR 1)
set(SDL_VERSION_MINOR 0)
set(SDL_VERSION_PATCH 0)
set(SDL_VERSION ${SDL_VERSION_MAJOR}.${SDL_VERSION_MINOR}.${SDL_VERSION_PATCH})

# guard against in-source builds
IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
ENDIF()

# Default build is in Release mode
IF(NOT CMAKE_BUILD_TYPE AND NOT MSVC)
  set(CMAKE_BUILD_TYPE "Release")
ENDIF(NOT CMAKE_BUILD_TYPE AND NOT MSVC)

#set debug mode by default, will be removed in final version
add_definitions(-DSDL_DEBUG)

#set the default path for built executables to the "bin" directory
#set the default path for built libraries to the "lib" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set( SDL_ADDITIONAL_INCLUDE src )

include_directories(${ADDITIONAL_INCLUDE_DIRS} ${SDL_ADDITIONAL_INCLUDE})

set( SDL_SOURCE_FILES
  src/software/main.cpp
  src/system/page_info.cpp
  src/system/page_head.cpp
  src/system/boot_page.cpp
  src/system/database.cpp
  src/system/file_h.cpp
  src/system/file_map.cpp
  src/system/sysallocunits.cpp
  src/system/syschobjs.cpp )

set( SDL_HEADER_FILES
  src/third_party/cmdLine/cmdLine.h
  src/common/config.h
  src/common/common.h
  src/common/noncopyable.h
  src/common/quantity.h
  src/common/static.h
  src/common/static_type.h
  src/common/type_seq.h
  src/common/typelist.h
  src/system/page_info.h
  src/system/page_head.h 
  src/system/boot_page.h
  src/system/database.h
  src/system/file_h.h
  src/system/file_map.h
  src/system/sysallocunits.h
  src/system/syschobjs.h )

add_executable( test_dataserver ${SDL_SOURCE_FILES} ${SDL_HEADER_FILES} )  