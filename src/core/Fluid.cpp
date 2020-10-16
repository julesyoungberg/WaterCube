#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name, ContainerRef container):
    BaseObject(name),
    container_(container)
{}

Fluid::~Fluid() {}

Fluid* Fluid::setup() {
    int n = num_particles_;
    particles_.resize(n);

    for (int i = 0; i < n; i++) {
        const float x = ((std::rand() % n) / (float)n);
        const float y = ((std::rand() % n) / (float)n);
        const float z = ((std::rand() % n) / (float)n);
        particles_[i] = Particle().position(vec3(x, y, z));
    }

    return this;
}

void Fluid::update(double time) {

}

void Fluid::draw() {

}

void Fluid::reset() {
    
}

FluidRef Fluid::create(const std::string& name, ContainerRef container) {
    return FluidRef(new Fluid(name, container));
}
