#!make
include .env
export $(shell sed 's/=.*//' .env)

.PHONY: run

run: build
	./build/Debug/WaterCube/WaterCube.app/Contents/MacOS/WaterCube

build: cmake
	pushd build && make -j4 && popd

cmake:
	mkdir -p build && pushd build && cmake ../proj/cmake && popd

clean:
	rm -rf build

format:
	clang-format --glob="./src/**/*.{h,cpp}" > /dev/null
