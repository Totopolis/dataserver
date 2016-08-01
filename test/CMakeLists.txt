cmake_minimum_required(VERSION 2.8)

include(ExternalProject)

###############################################################################
# Catch library
###############################################################################
# library is downloaded & updated on the fly
ExternalProject_Add(
    catch
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/catch
#    URL https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
    GIT_REPOSITORY https://github.com/philsquared/Catch.git
    TIMEOUT 10
    UPDATE_COMMAND git pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(catch source_dir)
set(CATCH_INCLUDE_DIR ${source_dir}/include
    CACHE INTERNAL
    "Path to include folder for Catch"
)

add_library(libcatch INTERFACE)

###############################################################################

#set(SDL_LIBRARIES dataserver)

#set(SDL_ADDITIONAL_INCLUDE 
#	externals/Catch 
#	externals/dataserver/src)

include_directories(
    ${CATCH_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}     # for generated dstest.h
)

#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


###############################################################################
# Generate dstest.h from dstest.sql
###############################################################################

# MDF to H compiler path
set(MDF_COMPILER_PATH
    ${BIN_PATH}/test_dataserver.exe
)    
MESSAGE(STATUS "MDF_COMPILER_PATH: ${MDF_COMPILER_PATH}")

file(GLOB SQL_FILE sql/dstest.sql)
file(TO_NATIVE_PATH ${SQL_FILE} SQL_FILE_N)

set(OUTPUT_DIR ${CMAKE_BINARY_DIR})
file(TO_NATIVE_PATH ${OUTPUT_DIR} OUTPUT_DIR_N)

file(TO_NATIVE_PATH ${OUTPUT_DIR}/dstest.mdf MDF_FILE_N)
file(TO_NATIVE_PATH ${OUTPUT_DIR}/dstest.ldf LDF_FILE_N)

set(DSTEST_H_FILE ${OUTPUT_DIR}/dstest.h)
file(TO_NATIVE_PATH ${DSTEST_H_FILE} DSTEST_H_FILE_N)

add_custom_command(
    OUTPUT ${DSTEST_H_FILE}
    # need to remove mdf & ldf, otherwise sql script fails
    COMMAND ${CMAKE_COMMAND} -E remove ${MDF_FILE_N} ${LDF_FILE_N}  
    COMMAND sqlcmd -i "${SQL_FILE_N}" -v database_dir="${OUTPUT_DIR_N}"
    COMMAND ${MDF_COMPILER_PATH} -i ${MDF_FILE_N} -o ${DSTEST_H_FILE_N} -w 1
    # depends from MDF to H compiler & SQL file
    # compiler must be built by the parent project
    DEPENDS test_dataserver ${SQL_FILE}
    COMMENT "Generate *.h"
)
set_source_files_properties(${DSTEST_H_FILE} PROPERTIES GENERATED true)

#set(DATASETS_DIR_PATH
#    ${PROJECT_SOURCE_DIR}/datasets)    
#add_custom_target(
#    copy_mdf
#    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dstest.mdf ${DATASETS_DIR_PATH}/
#    DEPENDS generate_mdf 
#)

#if(UNIX)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
#endif()

set(SOURCES 
	src/main.cpp)
   
add_executable(dstest ${SOURCES} ${DSTEST_H_FILE})
target_link_libraries(dstest libcatch dataserver)

add_test(
    NAME dstest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND dstest)
