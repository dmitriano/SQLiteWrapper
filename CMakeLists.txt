cmake_minimum_required (VERSION 3.6.2)
project (SQLiteWrapperTest)

set(AWL_ROOT_DIR "${CMAKE_SOURCE_DIR}/lib/Awl")
set(SQLITE_SRC_DIR $ENV{SQLITE_SRC_DIR})

include(CMake/AwlConfig.cmake)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${SQLITE_SRC_DIR})

file(GLOB_RECURSE CPP_FILES 
  ${SQLITE_SRC_DIR}/*.h ${SQLITE_SRC_DIR}/sqlite3.c
  ${CMAKE_SOURCE_DIR}/SQLiteWrapper/*.h ${CMAKE_SOURCE_DIR}/SQLiteWrapper/*.cpp
  ${CMAKE_SOURCE_DIR}/Tests/*.h ${CMAKE_SOURCE_DIR}/Tests/*.cpp)

add_executable(${PROJECT_NAME} ${CPP_FILES})

include(CMake/AwlLink.cmake)
