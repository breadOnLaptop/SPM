cmake_minimum_required(VERSION 3.10)
project(SPM)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

add_executable(spm ${SOURCES})

if(WIN32)
    target_link_libraries(spm psapi)
endif()
