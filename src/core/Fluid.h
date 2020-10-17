#pragma once

#include <memory>
#include <string>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/gl/Ssbo.h"

#include "./BaseObject.h"
#include "./Container.h"
#include "./Particle.h"

using namespace ci;
using namespace ci::app;

namespace core {

const int WORK_GROUP_SIZE = 128;

typedef std::shared_ptr<class Fluid> FluidRef;

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

private:
    int num_particles_;
    // std::vector<Particle> particles_;
    ContainerRef container_;
    gl::SsboRef particle_buffer_;
    gl::VboRef ids_vbo_;
    gl::VboRef attributes_;

};

};
