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
    Particle() : position(0), velocity(0), density(0), pressure(0) {}
    vec3 position;
    vec3 velocity;
    float density;
    float pressure;
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

protected:
    float size_, bin_size_, kernel_radius_;
    float particle_mass_;
    float viscosity_coefficient_;
    float viscosity_weight_, pressure_weight_, kernel_weight_;
    float stiffness_;
    float rest_density_, rest_pressure_;

    int num_particles_, grid_res_, num_work_groups_, num_bins_;

    bool odd_frame_;

    vec3 position_, gravity_;
    ContainerRef container_;
    std::vector<Particle> initial_particles_;

    gl::GlslProgRef bin_velocity_prog_, density_prog_, render_prog_, update_prog_, geometry_prog, shading_prog_;

    gl::SsboRef particle_buffer_1_, particle_buffer_2_, bin_velocity_buffer_;
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
