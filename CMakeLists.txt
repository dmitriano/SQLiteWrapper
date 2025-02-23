cmake_minimum_required (VERSION 3.6.2)
project (SQLiteWrapperTest)

set(AWL_ROOT_DIR "${CMAKE_SOURCE_DIR}/lib/Awl")
set(SQLITE_SRC_DIR $ENV{SQLITE_SRC_DIR})

if(NOT EXISTS "${SQLITE_SRC_DIR}/sqlite3.h")
    message( FATAL_ERROR "SQLite sources not found in ${SQLITE_SRC_DIR}" )
endif()

include(${AWL_ROOT_DIR}/CMake/AwlConfig.cmake)

#set(AWL_FIND_QT OFF)
#set(AWL_STATIC_RUNTIME ON)
set(AWL_COMPILE_MAIN ON)

file(GLOB_RECURSE CPP_FILES
  ${SQLITE_SRC_DIR}/*.h ${SQLITE_SRC_DIR}/sqlite3.c
  ${CMAKE_SOURCE_DIR}/SQLiteWrapper/*.h ${CMAKE_SOURCE_DIR}/SQLiteWrapper/*.cpp
  ${CMAKE_SOURCE_DIR}/Tests/*.h ${CMAKE_SOURCE_DIR}/Tests/*.cpp)

add_executable(${PROJECT_NAME} ${CPP_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${SQLITE_SRC_DIR})

include(${AWL_ROOT_DIR}/CMake/AwlLink.cmake)
