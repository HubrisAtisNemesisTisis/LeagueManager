cmake_minimum_required(VERSION 3.16)

#set(CMAKE_SYSTEM_NAME MSYS)
set(CMAKE_CXX_COMPILER  g++)

#Flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb3")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

#Project
project(LeagueofLegendsManager LANGUAGES CXX)
enable_language(CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(SIMDJSON REQUIRED simdjson)
message(STATUS "Simdjson found: ${SIMDJSON_FOUND}")
message(STATUS "Simdjson libraries: ${SIMDJSON_LIBRARIES}")
pkg_check_modules(SQLITE3 REQUIRED sqlite3)
message(STATUS "SQLITE3 found: ${SQLITE3_FOUND}")
message(STATUS "SQLITE3 libraries: ${SQLITE3_LIBRARIES}")

set(UCRT64_INCLUDE_DIRS /ucrt64/include)

add_executable(LeagueofLegendsManager check_if_client_open.cpp)
target_include_directories(LeagueofLegendsManager PRIVATE ${UCRT64_INCLUDE_DIRS})
target_link_libraries(
    LeagueofLegendsManager PRIVATE
        ${SQLITE3_LIBRARIES}
        ${SIMDJSON_LIBRARIES}
        ole32
        oleaut32
        uiautomationcore
)