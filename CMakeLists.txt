cmake_minimum_required(VERSION 3.30)
project(imrefl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imrefl INTERFACE)
target_include_directories(imrefl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (IMREFL_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()