# This is a hacky script for my use only - hence the relative path
# to my build of the Bloomberg clang fork with reflection.
#
# TODO: Have a proper solution for building this project

../clang-p2996/build-llvm/bin/clang++ \
  example.cpp \
  imgui/imgui.cpp \
  imgui/imgui_draw.cpp \
  imgui/imgui_tables.cpp \
  imgui/imgui_widgets.cpp \
  imgui/backends/imgui_impl_glfw.cpp \
  imgui/backends/imgui_impl_opengl3.cpp \
  -Iimgui \
  -Iimgui/backends \
  -lglfw \
  -lGL \
  -lX11 \
  -lXrandr \
  -lXi \
  -lXcursor \
  -lXinerama \
  -ldl \
  -lpthread \
  -std=c++26 \
  -stdlib=libc++ \
  -freflection \
  -fparameter-reflection \
  -fexpansion-statements \
  -fannotation-attributes \
  -ffixed-point \
  -O2
