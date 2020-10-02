#!make
include .env
export $(shell sed 's/=.*//' .env)

.PHONY: run

run: build
	open build/Debug/WaterCube/WaterCube.app

build: cmake
	pushd build && make -j4 && popd

cmake:
	mkdir -p build && pushd build && cmake ../proj/cmake && popd

clean:
	rm -rf build


