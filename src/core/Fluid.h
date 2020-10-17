#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"

#include "./BaseObject.h"
#include "./Container.h"

using namespace ci;
using namespace ci::app;

namespace core {

const int WORK_GROUP_SIZE = 128;

/**
 * Particle representation
 */
struct Particle {
    Particle() : position(0), velocity(0), density(1) {}
    vec3 position;
    vec3 velocity;
    float density;
};

typedef std::shared_ptr<class Fluid> FluidRef;

/**
 * Fluid Simulator class
 */
class Fluid : public BaseObject {
public:
    Fluid(const std::string& name, ContainerRef container);
    ~Fluid();

    int numParticles() { return num_particles_; }
    FluidRef numParticles(int n);

    FluidRef setup();
    void update(double time) override;
    void draw() override;
    void reset() override;

    static FluidRef create(const std::string& name, ContainerRef container);

    int num_particles_;
    ContainerRef container_;
    std::vector<Particle> initial_particles_;

    gl::GlslProgRef bucket_prog_, density_prog_, render_prog_, update_prog_;

    gl::SsboRef position_buffer_, velocity_buffer_, density_buffer_;
    gl::VboRef ids_vbo_;
    gl::VaoRef attributes_;

private:
    void generateInitialParticles();
    void prepareBuffers();

    void compileBucketProg();
    void compileDensityProg();
    void compileUpdateProg();
    void compileRenderProg();
    void compileShaders();

    void runProg();
    void runBucketProg();
    void runDensityProg();
    void runUpdateProg();
};

} // namespace core
