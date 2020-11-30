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
    FluidRef particleRadius(float r);
    FluidRef viscosityCoefficient(float c);
    FluidRef stiffness(float s);
    FluidRef restDensity(float d);
    FluidRef restPressure(float p);
    FluidRef position(vec3 p);
    FluidRef gravityStrength(float g);
    FluidRef renderMode(int m);

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
    void createParams();
    void generateInitialParticles();
    void prepareParticleBuffers();
    void prepareBuffers();

    void compileShaders();

    void transformWorldSpaceVectors();

    void runProg() { util::runProg(num_work_groups_); }
    void runDensityProg(GLuint particle_buffer);
    void runUpdateProg(GLuint particle_buffer, float time_step);
    void runAdvectProg(GLuint particle_buffer, float time_step);
    void renderParticles();

    FluidRef thisRef() { return std::make_shared<Fluid>(*this); }

    int num_particles_;
    int grid_res_;
    int num_bins_;
    int num_work_groups_;
    int render_mode_;

    float size_;
    float bin_size_;
    float kernel_radius_;
    float particle_mass_;
    float particle_radius_;
    float viscosity_coefficient_;
    float stiffness_;
    float rest_density_;
    float rest_pressure_;
    float gravity_strength_;
    float point_scale_;
    float dt_;
    float spiky_kernel_const_;
    float poly6_kernel_const_;
    float viscosity_kernel_const_;

    bool odd_frame_;
    bool first_frame_;
    bool rotate_gravity_;

    quat rotation_;

    vec3 position_;
    vec3 camera_position_;
    vec3 gravity_direction_;
    vec3 light_position_;

    ContainerRef container_;

    Ray mouse_ray_;

    std::vector<Particle> initial_particles_;
    std::vector<Plane> boundaries_;
    std::vector<ivec4> grid_particles_;

    gl::GlslProgRef density_prog_;
    gl::GlslProgRef update_prog_;
    gl::GlslProgRef render_particles_prog_;
    gl::GlslProgRef advect_prog_;

    SortRef sort_;
    MarchingCubeRef marching_cube_;

    GLuint particle_buffer1_;
    GLuint particle_buffer2_;
    GLuint vao1_, vao2_;
    GLuint debug_buffer_;

    params::InterfaceGlRef params_;
};

} // namespace core
