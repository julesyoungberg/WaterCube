# WaterCube

An interactive SPH water simulation using Cinder.


## Setup

Download cinder 0.9.3dev from github:

```shell
git clone --recursive https://github.com/cinder/Cinder.git
```


### Visual Studio 2019

Open `Cinder\CMakeLists.txt` in Visual Studio, once the build files have been generated select `Build > Build All`.

Next, update `CINDER_PATH` in `WaterCube\proj\cmake\CMakeSettings.json` to point to the cinder repo. 
Then open `WaterCube\proj\cmake\CMakeLists.txt` in Visual Studio and let the build files generate.
Lastly, either run `Current Document` as a startup item or select `Build > Build All` then run `WaterCube.exe` as a startup item.


### XCode

Open `Cinder/proj/xcode/cinder.xcodeproj` and build.

Next open `WaterCube/xcode/WaterCube.xcodeproj`, update `Header Search Paths` to include `/path-to-cinder/include`, then run.


### CMake

You must [build cinder](https://libcinder.org/docs/guides/cmake/cmake.html) with CMake before building this project. Then,

```shell
cp .env.example .env
```

Set `CINDER_PATH` in `.env`, then run

```shell
make
```

