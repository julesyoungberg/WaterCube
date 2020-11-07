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
#include "cinder/params/Params.h"

#include "./BaseObject.h"
#include "./Container.h"
#include "./Sort.h"
#include "./util.h"

using namespace ci;
using namespace ci::app;

namespace core {

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
    FluidRef gridRes(int r);
    FluidRef size(float s);
    FluidRef kernelRadius(float r);
    FluidRef particleMass(float m);
    FluidRef viscosityCoefficient(float c);
    FluidRef viscosityWeight(float w);
    FluidRef pressureWeight(float w);
    FluidRef kernelWeight(float w);
    FluidRef stiffness(float s);
    FluidRef restDensity(float d);
    FluidRef restPressure(float p);
    FluidRef position(vec3 p);
    FluidRef gravity(float g);

    void addParams(params::InterfaceGlRef p);

    FluidRef setup();
    void update(double time) override;
    void draw() override;
    void reset() override;

    static FluidRef create(const std::string& name) { return std::make_shared<Fluid>(name); }

protected:
    int num_particles_, grid_res_, num_work_groups_, num_bins_;

    float size_, bin_size_, kernel_radius_;
    float particle_mass_;
    float viscosity_coefficient_;
    float viscosity_weight_, pressure_weight_, kernel_weight_;
    float stiffness_;
    float rest_density_, rest_pressure_, gravity_;

    bool odd_frame_;

    vec3 position_;
    ContainerRef container_;
    std::vector<Particle> initial_particles_;
    std::vector<Plane> boundaries_;

    gl::GlslProgRef distance_field_prog_, bin_velocity_prog_, density_prog_, update_prog_;
    gl::GlslProgRef render_prog_, geometry_prog, shading_prog_;

    gl::SsboRef particle_buffer_1_, particle_buffer_2_, boundary_buffer_;
    gl::Texture1dRef wall_weight_function_;
    gl::Texture3dRef velocity_field_, distance_field_;
    gl::VboRef ids_vbo_;
    gl::VaoRef attributes_;

    SortRef sort_;

private:
    void generateInitialParticles();
    void generateBoundaryPlanes();
    void prepareWallWeightFunction();
    void prepareBuffers();

    void compileDistanceFieldProg();
    void compileBinVelocityProg();
    void compileDensityProg();
    void compileUpdateProg();
    void compileRenderProg();
    void compileShaders();

    void runProg(ivec3 work_groups);
    void runProg(int work_groups);
    void runProg();
    void runDistanceFieldProg();
    void runBinVelocityProg();
    void runDensityProg();
    void runUpdateProg(float time_step);
    void runRenderProg();

    FluidRef thisRef() { return std::make_shared<Fluid>(*this); }
};

} // namespace core
