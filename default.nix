{
  lib,
  cmake,
  imgui,
  gcc16Stdenv,
  glfw,
  glm,
  build_examples ? false,
}:
gcc16Stdenv.mkDerivation {
  pname = "imrefl";
  version = "0.0.1";
  cmakeFlags = [
    "-DIMREFL_BUILD_EXAMPLE=${if build_examples then "ON" else "OFF"}"
    "-DIMGUI_SOURCE_DIR=${imgui.src}"
  ];
  src = lib.cleanSource ./.;
  buildInputs = [
    cmake
    imgui
    glfw
    glm
  ];
}
