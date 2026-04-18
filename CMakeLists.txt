cmake_minimum_required(VERSION 3.14)
project(vmem-llm)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(vmem-llm src/main.cpp src/safetensors.cpp)
target_include_directories(vmem-llm PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(sandbox src/mmap_sandbox.cpp)
