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
#include "./Sort.h"

using namespace ci;
using namespace ci::app;

namespace core {

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
    Fluid(const std::string& name);
    ~Fluid();

    int numParticles() { return num_particles_; }
    FluidRef numParticles(int n);
    FluidRef particleMass(float m);

    FluidRef setup();
    void update(double time) override;
    void draw() override;
    void reset() override;

    static FluidRef create(const std::string& name);

    float size_, bin_size_, particle_mass_, kernel_, viscosity_coefficient_, weight_constant_;
    float viscosity_weight_, pressure_weight_, k_, rest_density_, rest_pressure_;
    int num_particles_, grid_res_, num_work_groups_, num_partitions_, num_bins_;

    vec3 position_, gravity_;
    ContainerRef container_;
    std::vector<Particle> initial_particles_;

    gl::GlslProgRef bin_velocity_prog_, density_prog_, render_prog_, update_prog_, geometry_prog, shading_prog_;

    gl::SsboRef position_buffer_, velocity_buffer_, density_buffer_, pressure_buffer_, bin_velocity_buffer_;
    gl::VboRef ids_vbo_;
    gl::VaoRef attributes_;

    SortRef sort_;

private:
    void generateInitialParticles();
    void prepareBuffers();

    void compileBinVelocityProg();
    void compileDensityProg();
    void compileUpdateProg();
    void compileRenderProg();
    void compileShaders();

    void runProg(int work_groups);
    void runProg();
    void runBinVelocityProg();
    void runDensityProg();
    void runUpdateProg(float time_step);
    void runRenderProg();
};

} // namespace core
