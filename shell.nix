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
    glfw3
    glew.dev
    libglvnd.dev
    eigen
  ];
  EM_CACHE =  "${builtins.unsafeDiscardStringContext (builtins.toString ./.)}/.em_cache";
}
