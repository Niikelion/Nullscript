cmake_minimum_required(VERSION 3.10)

project(Nullscript VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_library(nullscript STATIC ${SOURCES})
add_library(nullscript_d STATIC ${SOURCES})

set_target_properties(nullscript PROPERTIES 
			OUTPUT_NAME "nullscript"
			ARCHIVE_OUTPUT_DIRECTORY "bin/")

set_target_properties(nullscript_d PROPERTIES 
			OUTPUT_NAME "nullscript-d"
			ARCHIVE_OUTPUT_DIRECTORY "bin/")

target_link_libraries(nullscript   --static -03)
target_link_libraries(nullscript_d --static -g)