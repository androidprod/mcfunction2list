cmake_minimum_required(VERSION 3.15)
project(mcfunction2list VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mcfunction2list main.cpp)

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(mcfunction2list PRIVATE Threads::Threads)
endif()
