#include "./Fluid.h"

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
    // particles_.resize(n);

    std::vector<Particle> particles;
    particles.assign(n, Particle());

    // console() << "creating particles\n";
    for (int i = 0; i < n; i++) {
        const float x = ((std::rand() % n) / (float)n);
        const float y = ((std::rand() % n) / (float)n);
        const float z = ((std::rand() % n) / (float)n);
        auto &p = particles.at(i);
        p.position = vec3(x, y, z);
    }

    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= n / WORK_GROUP_SIZE);

    particle_buffer_ = gl::Ssbo::create(particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);
    gl::ScopedBuffer scoped_particle_ssbo(particle_buffer_);
    particle_buffer_->bindBase(0);

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
