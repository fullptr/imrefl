{
  description = "ImRefl - A C++26 reflection library for ImGui";

  inputs = {
    nixpkgs.url = "github:rucadi/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        library = pkgs.callPackage ./default.nix { };
        with_examples = pkgs.callPackage ./default.nix { build_examples = true; };
      in
      {
        packages.default = library;
        packages.with_examples = with_examples;
        apps.default = {
          type = "app";
          program = "${with_examples}/bin/imrefl-example";
        };
        devShells.default = pkgs.mkShell.override { stdenv = pkgs.gcc16Stdenv; } {
          packages = [
            pkgs.cmake
            pkgs.gcc16
            pkgs.glfw
            pkgs.glm
            pkgs.imgui
          ];
        };
      }
    );
}
