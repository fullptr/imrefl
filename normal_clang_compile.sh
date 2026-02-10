clang++ main.cpp \
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
  -std=c++17 \
  -O2

