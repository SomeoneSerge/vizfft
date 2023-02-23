{ lib
, stdenv
, cmake
, pkg-config
, eigen
, glew
, libglvnd
, SDL2
}:


let
  inherit (builtins) match;
in
stdenv.mkDerivation {
  pname = "vizfft";
  version = "0.0.1";

  src = lib.cleanSourceWith {
    filter = name: type:
      let
        clean = lib.cleanSourceFilter name type;
        baseName = baseNameOf (toString name);
        isBuildDir = type == "directory" && lib.hasPrefix "build" baseName;
      in
      clean && (!isBuildDir);
    src = ./.;
  };

  cmakeFlags = [
    "-DBUILD_EMSCRIPTEN=off"
  ];

  buildInputs = [
    SDL2
    glew.dev
    libglvnd.dev
    eigen
  ];

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  installPhase = ''
    install -Dt $out/bin/ ./vizfft
  '';
}
