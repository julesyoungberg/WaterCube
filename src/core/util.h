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

struct Plane {
    Plane() : normal(0), point(0) {}
    vec3 normal;
    vec3 point;
};

/**
 * Particle representation
 */
struct Particle {
    Particle() : position(0), velocity(0), density(0), pressure(0) {}
    vec3 position;
    vec3 velocity;
    float density;
    float pressure;
};

namespace util {

void log(char* format, ...);

void runProg(ivec3 work_groups);

void runProg(int work_groups);

gl::GlslProgRef compileComputeShader(char* filename);

std::vector<Particle> getParticles(gl::SsboRef particle_buffer, int num_items);

std::vector<uint32_t> getUints(gl::Texture1dRef tex, int num_items);

std::vector<uint32_t> getUints(gl::Texture3dRef tex, int num_items);

} // namespace util

} // namespace core
