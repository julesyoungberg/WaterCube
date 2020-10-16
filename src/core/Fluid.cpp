#include "cinder/app/App.h"

#include "./Fluid.h"

using namespace ci;
using namespace ci::app;
using namespace core;

Fluid::Fluid(const std::string& name, ContainerRef container):
    BaseObject(name),
    container_(container)
{}

Fluid::~Fluid() {}

FluidRef Fluid::numParticles(int n) {
    num_particles_ = n;
    return std::make_shared<Fluid>(*this);
}

FluidRef Fluid::setup() {
    // console() << "creating fluid\n";

    int n = num_particles_;
    particles_.resize(n);

    // console() << "creating particles\n";
    for (int i = 0; i < n; i++) {
        const float x = ((std::rand() % n) / (float)n);
        const float y = ((std::rand() % n) / (float)n);
        const float z = ((std::rand() % n) / (float)n);
        particles_[i] = Particle().position(vec3(x, y, z));
    }

    // console() << "fluid created\n";
    return std::make_shared<Fluid>(*this);
}

void Fluid::update(double time) {

}

void Fluid::draw() {

}

void Fluid::reset() {
    
}

FluidRef Fluid::create(const std::string& name, ContainerRef container) {
    return std::make_shared<Fluid>(name, container);
}
