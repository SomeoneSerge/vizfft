# vizfft

An imgui+webGL toy, visualizing FFT of [plane waves](https://en.wikipedia.org/wiki/Plane_wave).

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


## Roadmap

- [x] [init](https://github.com/SomeoneSerge/vizfft/commit/483365f50b3272b0e3931fbc8886836cdb192c18):
  a bunch of copy-paste to bootstrap the cmake, glfw, imgui, and emscripten trivia;
  raii headers in their current form can obviously only be included once, or they won't link correctly
- [x] [build](https://github.com/SomeoneSerge/vizfft/commit/58c3c47fda0d9b67f48dde7ff9b0f5fb1973bd9b) github-pages on a push to master
- [x] [patch](https://github.com/SomeoneSerge/vizfft/commit/c382d41238fe010f74c9f0096b4cbf952bd41366) the negative DeltaTime issue in firefox
- [x] [switch](https://github.com/SomeoneSerge/vizfft/commit/cf8b8e41959b455af689b21abc03365c7cccf309) from GLFW to SDL just to feel the difference between the APIs
- [ ] fft in a separate thread, on a frequency lower than the framerate
- [ ] switching between glfw/sdl via policies?
- [ ] a shader to visualize the implicit periodic extensions of the image
- [ ] an option to visualize an implicitly zero-padded signal
- [ ] 1d signal
