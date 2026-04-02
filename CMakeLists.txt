cmake_minimum_required(VERSION 3.30)
project(imrefl LANGUAGES CXX)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(IMREFL_BUILD_EXAMPLE "Build the example" ON)

add_library(ImRefl INTERFACE)
target_include_directories(ImRefl INTERFACE 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

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

install(TARGETS ImRefl EXPORT ImReflTargets)
install(FILES imrefl.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT ImReflTargets FILE ImReflConfig.cmake NAMESPACE ImRefl:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ImRefl)
