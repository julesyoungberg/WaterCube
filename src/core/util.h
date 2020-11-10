#pragma once

#include <Windows.h>
#include <cstdio>
#include <vector>

#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;

namespace core {

const int WORK_GROUP_SIZE = 128;

namespace util {

void log(char* format, ...);

void runProg(ivec3 work_groups);

void runProg(int work_groups);

gl::GlslProgRef compileComputeShader(char* filename);

} // namespace util

} // namespace core
