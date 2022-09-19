# vizfft

## Dependencies

0. Get https://nixos.org/download.html (`$ sh <(curl -L https://nixos.org/nix/install) --daemon`)
1. Get https://direnv.net/
3. Get https://github.com/nix-community/home-manager/ with `programs.direnv.enable = true` and `programs.direnv.nix-direnv.enable = true`
4. `echo use nix > .envrc`
5. `direnv allow`

## Build native

```console
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBUILD_EMSCRIPTEN=OFF
cmake --build build/
```

## Build WebGL


```console
cmake -B build -S . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build/
npx http-server build/ -c-1
```
