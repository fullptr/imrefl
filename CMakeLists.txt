cmake_minimum_required(VERSION 3.30)
project(imrefl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ImRefl INTERFACE)
target_include_directories(ImRefl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(ImRefl INTERFACE -freflection)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(ImRefl INTERFACE
    -fexpansion-statements
    -fannotation-attributes
    -fparameter-reflection
  )
endif()

if (IMREFL_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()
