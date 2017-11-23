cmake_minimum_required(VERSION 2.6)
project(dataserver C CXX)

set(SDL_UNIX_CXX_FLAGS "" CACHE STRING "more C++ compiler flags if UNIX")
message(STATUS "SDL_UNIX_CXX_FLAGS: ${SDL_UNIX_CXX_FLAGS}")

if(UNIX)
set(CMAKE_CXX_FLAGS "-Wall -std=c++14 ${SDL_UNIX_CXX_FLAGS}")
endif(UNIX)

set(SDL_VERSION_MAJOR 1)
set(SDL_VERSION_MINOR 0)
set(SDL_VERSION_REVISION 13)
set(SDL_VERSION ${SDL_VERSION_MAJOR}.${SDL_VERSION_MINOR}.${SDL_VERSION_REVISION})

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  message(FATAL_ERROR "In-source builds not allowed.")
endif()

# Default build is in Release mode
if(NOT CMAKE_BUILD_TYPE)
    if(MSVC)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif(MSVC)
endif(NOT CMAKE_BUILD_TYPE)

if(WIN32)
  message(STATUS "WIN32: ${WIN32}")
  add_definitions(-DSDL_OS_WIN32 -DWIN32)
  IF (MSVC)
    ADD_DEFINITIONS(/bigobj)
  ENDIF (MSVC)  
endif(WIN32)

if(UNIX)
  message(STATUS "UNIX: ${UNIX}")
  add_definitions(-DSDL_OS_UNIX)
endif(UNIX) 

if(APPLE)
  message(STATUS "APPLE: ${APPLE}")
  add_definitions(-DSDL_OS_APPLE)
  find_library(LIB_ICONV iconv)
endif(APPLE) 

add_definitions(-DSDL_VERSION_MAJOR=${SDL_VERSION_MAJOR})
add_definitions(-DSDL_VERSION_MINOR=${SDL_VERSION_MINOR})
add_definitions(-DSDL_VERSION_REVISION=${SDL_VERSION_REVISION})
add_definitions(-DSDL_VERSION=${SDL_VERSION})

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_definitions(-DSDL_DEBUG=1)
else()
    add_definitions(-DSDL_DEBUG=0)
endif()

configure_file( dataserver/system/version.h.in ${PROJECT_SOURCE_DIR}/dataserver/system/version.h )

#set the default path for built executables to the "bin" directory
#set the default path for built libraries to the "lib" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR: " ${CMAKE_CURRENT_SOURCE_DIR} )
include_directories(${ADDITIONAL_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

set( SDL_SOFTWARE_MAIN
  dataserver/software/main.cpp )

set( SDL_SOFTWARE_DUMP
  dataserver/software/dump.cpp )
  
set( SDL_HEADER_COMMON
  dataserver/common/stdcommon.h
  dataserver/common/common.h
  dataserver/common/config.h
  dataserver/common/config.cpp
  dataserver/common/noncopyable.h
  dataserver/common/singleton.h
  dataserver/common/quantity.h
  dataserver/common/static.h
  dataserver/common/array.h
  dataserver/common/vector_buf.h
  dataserver/common/array_enum.h
  dataserver/common/static_vector.h
  dataserver/common/format.h
  dataserver/common/static_type.h
  dataserver/common/type_seq.h
  dataserver/common/map_enum.h
  dataserver/common/compact_map.h
  dataserver/common/compact_set.h
  dataserver/common/outstream.h
  dataserver/common/locale.h
  dataserver/common/time_util.h
  dataserver/common/algorithm.h
  dataserver/common/hash_combine.h
  dataserver/common/to_string.h
  dataserver/common/indexx.h  #test
  dataserver/common/spinlock.h
  dataserver/common/thread.h
  )

set( SDL_SOURCE_COMMON
  dataserver/common/static.cpp
  dataserver/common/array.cpp
  dataserver/common/vector_buf.cpp
  dataserver/common/algorithm.cpp
  dataserver/common/compact_set.cpp
  dataserver/common/time_util.cpp
  dataserver/common/outstream.cpp
  dataserver/common/to_string.cpp
  dataserver/common/indexx.cpp  #test
  dataserver/common/spinlock.cpp 
  dataserver/common/thread.cpp
  )

set( SDL_HEADER_UTILS
  dataserver/utils/gregorian.hpp
  dataserver/utils/encoding_utf.hpp
  dataserver/utils/encoding_errors.hpp
  dataserver/utils/utf.hpp
  dataserver/utils/conv.h
  dataserver/utils/conv.cpp
  dataserver/utils/conv_unix.h
  dataserver/utils/conv_unix.cpp )
  
set( SDL_SOURCE_FILESYS
  dataserver/filesys/file_h.cpp
  dataserver/filesys/file_map.cpp
  dataserver/filesys/file_map_win32.cpp
  dataserver/filesys/file_map_unix.cpp )

set( SDL_HEADER_FILESYS
  dataserver/filesys/file_h.h
  dataserver/filesys/file_map.h
  dataserver/filesys/file_map_detail.h 
  dataserver/filesys/mmap64_unix.h  
  )

set( SDL_SOURCE_MAKETABLE
  dataserver/maketable/maketable.cpp
  dataserver/maketable/maketable_meta.cpp
  dataserver/maketable/generator.cpp
  dataserver/maketable/generator_util.cpp
  dataserver/maketable/export_database.cpp )

set( SDL_HEADER_MAKETABLE
  dataserver/maketable/maketable.h
  dataserver/maketable/maketable_select.hpp
  dataserver/maketable/maketable_scan.hpp
  dataserver/maketable/maketable_meta.h
  dataserver/maketable/maketable_base.h
  dataserver/maketable/maketable_where.h
  dataserver/maketable/generator.h
  dataserver/maketable/generator_util.h
  dataserver/maketable/export_database.h )

set( SDL_SOURCE_SPATIAL
  dataserver/spatial/spatial_type.cpp
  dataserver/spatial/spatial_index.cpp
  dataserver/spatial/spatial_tree.cpp
  dataserver/spatial/interval_cell.cpp
  dataserver/spatial/interval_set.cpp
  dataserver/spatial/sparse_set.cpp
  dataserver/spatial/transform.cpp
  dataserver/spatial/math_util.cpp
  dataserver/spatial/geo_data.cpp
  dataserver/spatial/geography.cpp
  dataserver/spatial/geography_info.cpp
  )

set( SDL_HEADER_SPATIAL
  dataserver/spatial/spatial_type.h
  dataserver/spatial/spatial_type.inl
  dataserver/spatial/hilbert.inl
  dataserver/spatial/spatial_index.h
  dataserver/spatial/spatial_tree.h
  dataserver/spatial/spatial_tree_t.h
  dataserver/spatial/spatial_tree_t.hpp
  dataserver/spatial/sparse_set.h
  dataserver/spatial/sparse_set.hpp
  dataserver/spatial/interval_cell.h
  dataserver/spatial/interval_cell.hpp
  dataserver/spatial/interval_set.h
  dataserver/spatial/interval_set.hpp
  dataserver/spatial/function_cell.h
  dataserver/spatial/function_cell.cpp
  dataserver/spatial/merge_cells.h
  dataserver/spatial/transform.h
  dataserver/spatial/transform.inl
  dataserver/spatial/math_util.h
  dataserver/spatial/geo_data.h
  dataserver/spatial/geography.h
  dataserver/spatial/geography_info.h
  )

set( SDL_SOURCE_BPOOL
  dataserver/bpool/vm_base.h
  dataserver/bpool/vm_win32.h
  dataserver/bpool/vm_win32.cpp
  dataserver/bpool/vm_unix.h
  dataserver/bpool/vm_unix.inl
  dataserver/bpool/vm_unix.cpp  
  dataserver/bpool/block_head.h
  dataserver/bpool/block_head.inl
  dataserver/bpool/block_head.cpp
  dataserver/bpool/block_list.h
  dataserver/bpool/block_list.inl
  dataserver/bpool/block_list.cpp  
  dataserver/bpool/page_bpool.cpp
  dataserver/bpool/page_bpool.h
  dataserver/bpool/page_bpool.inl
  dataserver/bpool/thread_id.h
  dataserver/bpool/thread_id.inl
  dataserver/bpool/thread_id.cpp
  dataserver/bpool/alloc_unix.h
  dataserver/bpool/alloc_unix.cpp
  dataserver/bpool/alloc_win32.h
  dataserver/bpool/alloc_win32.inl
  dataserver/bpool/alloc_win32.cpp
  dataserver/bpool/file.h
  dataserver/bpool/file.cpp
  dataserver/bpool/flag_type.h
  )
  
set( SDL_SOURCE_SYSTEM
  dataserver/system/mem_utils.cpp
  dataserver/system/page_type.cpp
  dataserver/system/page_info.cpp
  dataserver/system/page_head.cpp
  dataserver/system/datapage.cpp
  dataserver/system/database.cpp
  dataserver/system/database_impl.cpp
  dataserver/system/datatable.cpp
  dataserver/system/overflow.cpp
  dataserver/system/page_map.cpp
  dataserver/system/index_page.cpp
  dataserver/system/index_tree.cpp
  dataserver/system/primary_key.cpp
  dataserver/system/usertable.cpp
  )

set( SDL_HEADER_SYSTEM
  dataserver/system/version.h
  dataserver/system/mem_utils.h
  dataserver/system/page_type.h
  dataserver/system/page_meta.h
  dataserver/system/page_info.h
  dataserver/system/type_utf.h
  dataserver/system/page_head.h
  dataserver/system/page_head.inl
  dataserver/system/datapage.h
  dataserver/system/database.h
  dataserver/system/database.inl
  dataserver/system/database_cfg.h
  dataserver/system/database_fwd.h
  dataserver/system/database_impl.h
  dataserver/system/datatable.h
  dataserver/system/datatable.inl
  dataserver/system/overflow.h
  dataserver/system/page_map.h
  dataserver/system/slot_iterator.h
  dataserver/system/page_iterator.h
  dataserver/system/scalartype_t.h
  dataserver/system/index_page.h
  dataserver/system/index_tree.h
  dataserver/system/index_tree.inl
  dataserver/system/index_tree_t.h
  dataserver/system/index_tree_t.inl
  dataserver/system/index_tree_t.hpp
  dataserver/system/primary_key.h
  dataserver/system/usertable.h
  )

set( SDL_SOURCE_SYSOBJ
  dataserver/sysobj/boot_page.cpp
  dataserver/sysobj/file_header.cpp
  dataserver/sysobj/sysallocunits.cpp
  dataserver/sysobj/sysschobjs.cpp
  dataserver/sysobj/syscolpars.cpp
  dataserver/sysobj/sysidxstats.cpp
  dataserver/sysobj/sysiscols.cpp
  dataserver/sysobj/sysobjvalues.cpp
  dataserver/sysobj/sysscalartypes.cpp
  dataserver/sysobj/sysrowsets.cpp
  dataserver/sysobj/iam_page_row.cpp
  dataserver/sysobj/iam_page.cpp
  dataserver/sysobj/pfs_page.cpp )

set( SDL_HEADER_SYSOBJ
  dataserver/sysobj/boot_page.h
  dataserver/sysobj/file_header.h
  dataserver/sysobj/sysallocunits.h
  dataserver/sysobj/sysschobjs.h
  dataserver/sysobj/syscolpars.h
  dataserver/sysobj/sysidxstats.h
  dataserver/sysobj/sysiscols.h
  dataserver/sysobj/sysobjvalues.h
  dataserver/sysobj/sysscalartypes.h
  dataserver/sysobj/sysrowsets.h
  dataserver/sysobj/iam_page_row.h
  dataserver/sysobj/iam_page.h
  dataserver/sysobj/iam_page.inl
  dataserver/sysobj/pfs_page.h )
  
set( SDL_HEADER_THIRD_PARTY
  dataserver/third_party/cmdLine/cmdLine.h )

set( SDL_SOURCE_FILES
  ${SDL_SOURCE_COMMON}
  ${SDL_SOURCE_FILESYS} 
  ${SDL_SOURCE_SYSTEM}
  ${SDL_SOURCE_SPATIAL}
  ${SDL_SOURCE_SYSOBJ}
  ${SDL_SOURCE_MAKETABLE}
  ${SDL_SOURCE_BPOOL}
  )

set( SDL_HEADER_FILES
  ${SDL_HEADER_THIRD_PARTY}
  ${SDL_HEADER_COMMON}
  ${SDL_HEADER_UTILS}
  ${SDL_HEADER_FILESYS}
  ${SDL_HEADER_SYSTEM}
  ${SDL_HEADER_SPATIAL}
  ${SDL_HEADER_SYSOBJ}
  ${SDL_HEADER_MAKETABLE}
  )

source_group(third_party FILES ${SDL_HEADER_THIRD_PARTY} )
source_group(common FILES ${SDL_HEADER_COMMON} ${SDL_SOURCE_COMMON} )
source_group(utils FILES ${SDL_HEADER_UTILS} )
source_group(filesys FILES ${SDL_SOURCE_FILESYS} ${SDL_HEADER_FILESYS} )
source_group(system FILES ${SDL_SOURCE_SYSTEM} ${SDL_HEADER_SYSTEM} )
source_group(spatial FILES ${SDL_SOURCE_SPATIAL} ${SDL_HEADER_SPATIAL} )
source_group(sysobj FILES ${SDL_SOURCE_SYSOBJ} ${SDL_HEADER_SYSOBJ} )
source_group(maketable FILES ${SDL_SOURCE_MAKETABLE} ${SDL_HEADER_MAKETABLE} )
source_group(bpool FILES ${SDL_SOURCE_BPOOL} )
source_group(software FILES ${SDL_SOFTWARE_MAIN} ${SDL_SOFTWARE_DUMP} )

add_library( dataserver STATIC ${SDL_SOURCE_FILES} ${SDL_HEADER_FILES} )
add_executable( test_dataserver ${SDL_SOFTWARE_MAIN} ${SDL_SOURCE_FILES} ${SDL_HEADER_FILES} ) #include sources to call unit_test(s)

if(APPLE)
target_link_libraries(test_dataserver ${LIB_ICONV})
target_link_libraries(dataserver ${LIB_ICONV})
endif(APPLE) 

if(UNIX)
target_link_libraries(test_dataserver -lpthread)
target_link_libraries(dataserver -lpthread)
endif(UNIX) 