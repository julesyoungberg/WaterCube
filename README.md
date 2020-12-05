# WaterCube

A GPU powered SPH water simulation built on OpenGL with Cinder.

demo: https://www.youtube.com/watch?v=w6Nh7prwYbY

## Setup

Windows only sorry

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

## Running

Once the application starts press `s` to start the simulation.
