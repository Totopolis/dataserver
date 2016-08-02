# Windows: cmake -G "Visual Studio 14 Win64" ..

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(dataserver C CXX)

# to turn on: cmake -DSDL_DEBUG=ON
OPTION(SDL_DEBUG "Build with defined SDL_DEBUG macro" OFF)

###############################################################################
# 'dataserver' library version
###############################################################################
set(SDL_VERSION_MAJOR 1)
set(SDL_VERSION_MINOR 0)
set(SDL_VERSION_REVISION 0)
set(SDL_VERSION ${SDL_VERSION_MAJOR}.${SDL_VERSION_MINOR}.${SDL_VERSION_REVISION})

###############################################################################
# Build type
###############################################################################
#if(NOT CMAKE_BUILD_TYPE AND NOT MSVC)
if(NOT CMAKE_BUILD_TYPE)
  #set(CMAKE_BUILD_TYPE "Release")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message(STATUS "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL_DEBUG: ${SDL_DEBUG}")
message(STATUS "")

###############################################################################
# Compiler flags and definitions
###############################################################################
if(UNIX)
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
endif()

if(WIN32)
    add_definitions(-DSDL_OS_WIN32 -DWIN32)
endif()

if(UNIX)
    add_definitions(-DSDL_OS_UNIX)
endif() 

if(SDL_DEBUG MATCHES "ON")
    add_definitions(-DSDL_DEBUG=1)  # verbose debug output
endif()

###############################################################################
# Version
###############################################################################
add_definitions(-DSDL_VERSION_MAJOR=${SDL_VERSION_MAJOR})
add_definitions(-DSDL_VERSION_MINOR=${SDL_VERSION_MINOR})
add_definitions(-DSDL_VERSION_REVISION=${SDL_VERSION_REVISION})
add_definitions(-DSDL_VERSION=${SDL_VERSION})
configure_file( src/system/version.h.in ${PROJECT_SOURCE_DIR}/src/system/version.h )
 
#set the default path for built executables to the "bin" directory
#set the default path for built libraries to the "lib" directory
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(BIN_PATH ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}
    CACHE STRING
    ""
) 
MESSAGE(STATUS "BIN_PATH: ${BIN_PATH}")    


set(SDL_ADDITIONAL_INCLUDE src)

include_directories(
    ${ADDITIONAL_INCLUDE_DIRS}
    ${SDL_ADDITIONAL_INCLUDE}
)

set( SDL_SOURCE_SOFTWARE
  src/software/main.cpp )

set( SDL_HEADER_COMMON
  src/common/config.h
  src/common/config.cpp
  src/common/common.h
  src/common/noncopyable.h
  src/common/quantity.h
  src/common/array.h
  src/common/static.h
  src/common/static_type.h
  src/common/type_seq.h
  src/common/map_enum.h
  src/common/outstream.h
  src/common/locale.h
  src/common/time_span.h )

set( SDL_SOURCE_COMMON
  src/common/static.cpp )
  
set( SDL_SOURCE_FILESYS
  src/filesys/file_h.cpp
  src/filesys/file_map.cpp
  src/filesys/file_map_win32.cpp
  src/filesys/file_map_unix.cpp )

set( SDL_HEADER_FILESYS
  src/filesys/file_h.h
  src/filesys/file_map.h
  src/filesys/file_map_detail.h )

set( SDL_SOURCE_MAKETABLE
  src/maketable/maketable.cpp
  src/maketable/maketable_meta.cpp
  src/maketable/generator.cpp )

set( SDL_HEADER_MAKETABLE
  src/maketable/maketable.h
  src/maketable/maketable_select.hpp
  src/maketable/maketable_scan.hpp
  src/maketable/maketable_meta.h
  src/maketable/maketable_base.h
  src/maketable/maketable_where.h  
  src/maketable/generator.h )
  
set( SDL_SOURCE_SYSTEM
  src/system/page_type.cpp
  src/system/page_info.cpp
  src/system/page_head.cpp
  src/system/datapage.cpp
  src/system/database.cpp
  src/system/datatable.cpp
  src/system/overflow.cpp
  src/system/page_map.cpp
  src/system/index_page.cpp
  src/system/index_tree.cpp
  src/system/spatial_type.cpp
  src/system/spatial_index.cpp
  src/system/transform.cpp
  src/system/math_util.cpp
  src/system/geography.cpp
  src/system/geography_info.cpp
  src/system/primary_key.cpp
  src/system/usertable.cpp )

set( SDL_HEADER_SYSTEM
  src/system/version.h
  src/system/page_type.h
  src/system/page_meta.h
  src/system/page_info.h
  src/system/page_head.h
  src/system/page_head.inl
  src/system/datapage.h
  src/system/database.h
  src/system/database.inl
  src/system/database_fwd.h
  src/system/datatable.h
  src/system/datatable.inl
  src/system/overflow.h
  src/system/page_map.h
  src/system/slot_iterator.h
  src/system/page_iterator.h
  src/system/index_page.h
  src/system/index_tree.h
  src/system/index_tree.inl
  src/system/index_tree_t.h
  src/system/index_tree_t.inl
  src/system/index_tree_t.hpp
  src/system/spatial_type.h
  src/system/spatial_type.inl
  src/system/hilbert.inl
  src/system/spatial_index.h
  src/system/spatial_tree_t.h
  src/system/spatial_tree_t.hpp
  src/system/transform.h
  src/system/transform.inl
  src/system/math_util.h
  src/system/geography.h
  src/system/geography_info.h
  src/system/primary_key.h
  src/system/usertable.h )

set( SDL_SOURCE_SYSOBJ
  src/sysobj/boot_page.cpp
  src/sysobj/file_header.cpp
  src/sysobj/sysallocunits.cpp
  src/sysobj/sysschobjs.cpp
  src/sysobj/syscolpars.cpp
  src/sysobj/sysidxstats.cpp
  src/sysobj/sysiscols.cpp
  src/sysobj/sysobjvalues.cpp
  src/sysobj/sysscalartypes.cpp
  src/sysobj/sysrowsets.cpp
  src/sysobj/iam_page_row.cpp
  src/sysobj/iam_page.cpp
  src/sysobj/pfs_page.cpp )

set( SDL_HEADER_SYSOBJ
  src/sysobj/boot_page.h
  src/sysobj/file_header.h
  src/sysobj/sysallocunits.h
  src/sysobj/sysschobjs.h
  src/sysobj/syscolpars.h
  src/sysobj/sysidxstats.h
  src/sysobj/sysiscols.h
  src/sysobj/sysobjvalues.h
  src/sysobj/sysscalartypes.h
  src/sysobj/sysrowsets.h
  src/sysobj/iam_page_row.h
  src/sysobj/iam_page.h
  src/sysobj/pfs_page.h )
  
set( SDL_HEADER_THIRD_PARTY
  src/third_party/cmdLine/cmdLine.h )

set( SDL_SOURCE_FILES
  ${SDL_SOURCE_COMMON}
  ${SDL_SOURCE_FILESYS} 
  ${SDL_SOURCE_SYSTEM}
  ${SDL_SOURCE_SYSOBJ}
  ${SDL_SOURCE_MAKETABLE} )

set( SDL_HEADER_FILES
  ${SDL_HEADER_THIRD_PARTY}
  ${SDL_HEADER_COMMON}
  ${SDL_HEADER_FILESYS}
  ${SDL_HEADER_SYSTEM}
  ${SDL_HEADER_SYSOBJ}
  ${SDL_HEADER_MAKETABLE} )

source_group(third_party FILES ${SDL_HEADER_THIRD_PARTY} )
source_group(common FILES ${SDL_HEADER_COMMON} ${SDL_SOURCE_COMMON} )
source_group(software FILES ${SDL_SOURCE_SOFTWARE} )
source_group(filesys FILES ${SDL_SOURCE_FILESYS} ${SDL_HEADER_FILESYS} )
source_group(system FILES ${SDL_SOURCE_SYSTEM} ${SDL_HEADER_SYSTEM} )
source_group(sysobj FILES ${SDL_SOURCE_SYSOBJ} ${SDL_HEADER_SYSOBJ} )
source_group(maketable FILES ${SDL_SOURCE_MAKETABLE} ${SDL_HEADER_MAKETABLE} )

add_library(dataserver STATIC ${SDL_SOURCE_FILES} ${SDL_HEADER_FILES})

add_executable(test_dataserver ${SDL_SOURCE_SOFTWARE})
target_link_libraries(test_dataserver dataserver)

enable_testing()
add_subdirectory(test)
