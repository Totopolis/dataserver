cmake_minimum_required(VERSION 2.6)
project(dataserver C CXX)

# support C++11 with Clang
if(UNIX)
  set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
endif(UNIX)

set(SDL_VERSION_MAJOR 1)
set(SDL_VERSION_MINOR 0)
set(SDL_VERSION_PATCH 0)
set(SDL_VERSION ${SDL_VERSION_MAJOR}.${SDL_VERSION_MINOR}.${SDL_VERSION_PATCH})

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

# Default build is in Release mode
if(NOT CMAKE_BUILD_TYPE AND NOT MSVC)
  #set(CMAKE_BUILD_TYPE "Release")
  set(CMAKE_BUILD_TYPE "Debug")
endif(NOT CMAKE_BUILD_TYPE AND NOT MSVC)

if(WIN32)
  add_definitions(-DSDL_OS_WIN32 -DWIN32)
endif(WIN32)

if(UNIX)
  add_definitions(-DSDL_OS_UNIX)
endif(UNIX) 

#set debug mode by default, will be removed in final version
add_definitions(-DSDL_DEBUG)

#set the default path for built executables to the "bin" directory
#set the default path for built libraries to the "lib" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set( SDL_ADDITIONAL_INCLUDE src )

include_directories(${ADDITIONAL_INCLUDE_DIRS} ${SDL_ADDITIONAL_INCLUDE})

set( SDL_SOURCE_SOFTWARE
  src/software/main.cpp )
  
set( SDL_SOURCE_SYSTEM
  src/system/page_type.cpp
  src/system/page_info.cpp
  src/system/page_head.cpp
  src/system/boot_page.cpp
  src/system/file_header.cpp
  src/system/datapage.cpp
  src/system/database.cpp
  src/system/file_h.cpp
  src/system/file_map.cpp
  src/system/file_map_win32.cpp
  src/system/file_map_unix.cpp
  src/system/sysallocunits.cpp
  src/system/sysschobjs.cpp
  src/system/syscolpars.cpp
  src/system/sysidxstats.cpp
  src/system/sysiscols.cpp
  src/system/sysobjvalues.cpp
  src/system/sysscalartypes.cpp )  

set( SDL_HEADER_COMMON
  src/common/config.h
  src/common/common.h
  src/common/noncopyable.h
  src/common/quantity.h
  src/common/static.h
  src/common/static_type.h
  src/common/type_seq.h )

set( SDL_HEADER_SYSTEM
  src/system/page_type.h
  src/system/page_meta.h
  src/system/page_info.h
  src/system/page_head.h 
  src/system/boot_page.h
  src/system/file_header.h
  src/system/datapage.h
  src/system/database.h
  src/system/file_h.h
  src/system/file_map.h
  src/system/file_map_detail.h
  src/system/sysallocunits.h
  src/system/sysschobjs.h
  src/system/syscolpars.h
  src/system/sysidxstats.h
  src/system/sysiscols.h
  src/system/sysobjvalues.h
  src/system/sysscalartypes.h )  

set( SDL_HEADER_THIRD_PARTY
  src/third_party/cmdLine/cmdLine.h )
  
set( SDL_SOURCE_FILES
  ${SDL_SOURCE_SOFTWARE}
  ${SDL_SOURCE_SYSTEM} )

set( SDL_HEADER_FILES
  ${SDL_HEADER_THIRD_PARTY}
  ${SDL_HEADER_COMMON}
  ${SDL_HEADER_SYSTEM} )

source_group(third_party FILES ${SDL_HEADER_THIRD_PARTY} )  
source_group(common FILES ${SDL_HEADER_COMMON} )  
source_group(software FILES ${SDL_SOURCE_SOFTWARE} )  
source_group(system FILES ${SDL_SOURCE_SYSTEM} ${SDL_HEADER_SYSTEM} )  
  
add_executable( test_dataserver ${SDL_SOURCE_FILES} ${SDL_HEADER_FILES} )  