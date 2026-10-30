cmake_minimum_required(VERSION 3.5.0) # Bo to mam

project(Astroinfa LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VERBOSE true)

include(FetchContent)

FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        master
)

FetchContent_MakeAvailable(jsoncpp)

add_executable(
    "zad1_exec" "lab1/lab1.cpp"
)
target_include_directories("zad1_exec" PUBLIC ${jsoncpp_INCLUDE_DIR})
target_link_libraries(zad1_exec PUBLIC jsoncpp_static aio)

# So that clangd sees things installed by cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)
# endif()