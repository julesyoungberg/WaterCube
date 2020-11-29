#pragma once

#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "./BaseObject.h"
#include "./Container.h"
#include "./MarchingCube.h"
#include "./Sort.h"
#include "./util.h"

using namespace ci;
using namespace ci::app;

namespace core {

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
    FluidRef particleRadius(float r);
    FluidRef viscosityCoefficient(float c);
    FluidRef viscosityWeight(float w);
    FluidRef pressureWeight(float w);
    FluidRef kernelWeight(float w);
    FluidRef stiffness(float s);
    FluidRef restDensity(float d);
    FluidRef restPressure(float p);
    FluidRef position(vec3 p);
    FluidRef gravityStrength(float g);
    FluidRef renderMode(int m);
    FluidRef sortInterval(int i);

    void addParams(params::InterfaceGlRef p);

    void setCameraPosition(vec3 p);
    void setLightPosition(vec3 p);
    void setRotation(quat r);
    void setMouseRay(Ray r);

    FluidRef setup();
    void update(double time) override;
    void draw() override;
    void reset() override {}

    static FluidRef create(const std::string& name) { return std::make_shared<Fluid>(name); }

protected:
    void generateInitialParticles();
    void prepareParticleBuffers();
    void prepareBuffers();

    void compileShaders();

    void runProg() { util::runProg(num_work_groups_); }
    void runDensityProg(GLuint particle_buffer);
    void runUpdateProg(GLuint particle_buffer, float time_step);
    void runAdvectProg(GLuint particle_buffer, float time_step);
    void renderParticles();

    FluidRef thisRef() { return std::make_shared<Fluid>(*this); }

    int num_particles_, grid_res_;
    int num_work_groups_, num_bins_;
    int render_mode_;
    int sort_interval_;

    float size_, bin_size_, kernel_radius_;
    float particle_mass_, particle_radius_;
    float viscosity_coefficient_;
    float viscosity_weight_, pressure_weight_, kernel_weight_;
    float stiffness_;
    float rest_density_, rest_pressure_, gravity_strength_;
    float point_scale_;
    float dt_;

    bool odd_frame_, first_frame_;

    quat rotation_;

    vec3 position_, camera_position_, gravity_direction_, light_position_;

    ContainerRef container_;

    Ray mouse_ray_;

    std::vector<Particle> initial_particles_;
    std::vector<Plane> boundaries_;
    std::vector<ivec4> grid_particles_;

    gl::GlslProgRef density_prog_, update_prog_;
    gl::GlslProgRef render_particles_prog_;
    gl::GlslProgRef advect_prog_;

    SortRef sort_;
    MarchingCubeRef marching_cube_;

    GLuint particle_buffer1_, particle_buffer2_;
    GLuint vao1_, vao2_;
    GLuint debug_buffer_;
};

} // namespace core
