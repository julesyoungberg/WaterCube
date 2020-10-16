#include "./Particle.h"

using namespace core;

Particle::Particle(): Object("") {}

ParticleRef Particle::create() {
    return ParticleRef(new Particle());
}
