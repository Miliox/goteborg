# Göteborg

Laid-back Modern Gameboy Emulator.

## How to build?

This project uses:

* [cmake](http://www.cmake.org/)
* [conan](https://conan.io/)
* [bincrafters](https://bincrafters.github.io/2017/06/06/using-bincrafters-conan-repository/)
* [sfml](https://www.sfml-dev.org/)
* [imgui](https://github.com/ocornut/imgui)
* [imgui-sfml](https://github.com/eliasdaler/imgui-sfml)

```sh
cd build

# Setup once
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan

# Setup
conan install ..
cmake ..

# Build
make -j$(nproc)

# Run
bin/goteborg
```


