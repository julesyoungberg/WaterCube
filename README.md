# WaterCube

An interactive SPH water simulation using Cinder.

## Setup

The simplest way to get started is to download cinder 0.9.1 from the [download page](https://libcinder.org/download).

Works on [Mac](https://libcinder.org/docs/guides/mac-setup/index.html) or [Windows](https://libcinder.org/docs/guides/windows-setup/index.html).

### Setting the Path

This project needs to know where cinder is installed. 

#### Visual Studio

#### XCode
Update `Header Search Paths` to include `/path-to-cinder/include`.

#### CMake

```shell
cp .env.example .env
```

Set `CINDER_PATH` in `.env` to the path to cinder.
