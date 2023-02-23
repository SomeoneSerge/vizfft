let
  some = builtins.getFlake github:SomeoneSerge/pkgs;
  pkgs = import <nixpkgs> {
    # config.cudaSupport = true;
    # config.allowUnfree = true;
    overlays = [ some.overlay ];
  };
in
with pkgs;
mkShell {
  packages = [
    emscripten
    cmake
    pkg-config
    ninja
    rsync
  ];
  buildInputs = [
    # glfw3
    SDL2.dev
    glew.dev
    libglvnd.dev
    eigen
  ];
  EM_CACHE = "${builtins.unsafeDiscardStringContext (builtins.toString ./.)}/.em_cache";
  EM_PREBUILT_CACHE = "${emscripten}/share/emscripten/cache/";
}
