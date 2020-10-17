#pragma once

// #include <memory>
// #include <string>

#include "cinder/gl/gl.h"

// #include "./Object.h"

using namespace ci;

namespace core {

// typedef std::shared_ptr<class Particle> ParticleRef;

// class Particle : public Object<Particle> {
// public:
//     Particle();

//     double density() { return density_; }
//     Particle& density(double density);

//     double pressure() { return pressure_; }
//     Particle& pressure(double pressure);

// private:
//     double density_, pressure_;

// };

struct Particle {
    Particle(): position(0), velocity(0), density(1), pressure(1) {}

    vec3 position, velocity;
    float density, pressure;
}

};
