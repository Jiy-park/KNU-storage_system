cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME BAST)
set(CMAKE_CXX_STANDARD 17)

project(${PROJECT_NAME})
add_executable(${PROJECT_NAME}
src/main.cpp
src/FLASH_MEMORY.cpp src/flash_memory.h
src/FTL.cpp src/ftl.h 
)