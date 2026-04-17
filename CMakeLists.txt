cmake_minimum_required(VERSION 3.14)
project(vmem-llm)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include_directories(include)
add_executable(vmem-llm src/main.cpp src/safetensors.cpp)

add_executable(sandbox src/mmap_sandbox.cpp)
