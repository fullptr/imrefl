find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
)
message("Fetching ImGui...")
FetchContent_MakeAvailable(imgui)

set(IMGUI_SRC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(imrefl-example example.cpp ${IMGUI_SRC})

target_include_directories(imrefl-example PRIVATE ${imgui_SOURCE_DIR})

# Clang specific flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(imrefl-example PRIVATE
    -stdlib=libc++
  )

  # Hack to get the runtime library path
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -stdlib=libc++ -print-file-name=libc++.so
    OUTPUT_VARIABLE LIBCXX_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  cmake_path(GET LIBCXX_PATH PARENT_PATH CLANG_RPATH)

  target_link_options(imrefl-example PRIVATE 
    -stdlib=libc++
    -Wl,-rpath,${CLANG_RPATH}
  )
endif()

target_link_libraries(imrefl-example PRIVATE
  ${OPENGL_LIBRARIES}
  glfw
  ImRefl
)