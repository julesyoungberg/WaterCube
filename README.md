# WaterCube

A GPU powered SPH water simulation using Cinder.

## Setup


```shell
git clone --recursive https://github.com/julesyoungberg/WaterCube
```

### Visual Studio 2019

Open `lib/Cinder/CMakeLists.txt` in Visual Studio, once the build files have been generated select `Build > Build All`.

Then open `WaterCube/proj/cmake/CMakeLists.txt` in Visual Studio and let the build files generate.
Lastly, either run `Current Document` as a startup item or select `Build > Build All` then run `WaterCube.exe` as a startup item.

### CMake

You must [build cinder](https://libcinder.org/docs/guides/cmake/cmake.html) with CMake in `./lib/Cinder` before building this project. Then,

```shell
cp .env.example .env
make
```

## Todos

- sort only every nth frame
- use sorted grid as base for marching cube, but further subdivide
