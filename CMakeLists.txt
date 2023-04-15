cmake_minimum_required (VERSION 3.6.2)
project (SQLiteWrapperTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  add_definitions("-Wall -Wextra -pedantic -pthread")
  #usign GCC sanitizer:
  #add_definitions("-fsanitize=thread -fno-omit-frame-pointer")
  #link_libraries("-fsanitize=thread")
  #add_definitions("-fsanitize=undefined -fno-omit-frame-pointer")
  #link_libraries("-fsanitize=undefined")
  #add_definitions("-fsanitize=address -fno-omit-frame-pointer")
  #link_libraries("-fsanitize=address")
  #set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=gold")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  add_compile_options("/W4")
  add_definitions(-D_UNICODE)
  add_definitions(-MP)
endif()

set(AWL_ROOT_DIR "../Awl")
set(AWL_DIR ${AWL_ROOT_DIR}/Awl)
set(SQLITE_SRC_DIR $ENV{SQLITE_SRC_DIR})

include_directories("." ${AWL_ROOT_DIR} ${SQLITE_SRC_DIR})

find_package (Threads)

file(GLOB_RECURSE CPP_FILES 
  ${AWL_DIR}/*.h ${AWL_DIR}/*.cpp
  ${SQLITE_SRC_DIR}/*.h ${SQLITE_SRC_DIR}/sqlite3.c
  ${CMAKE_SOURCE_DIR}/*.h ${CMAKE_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${CPP_FILES})

target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
