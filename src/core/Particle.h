#pragma once

#include <memory>
#include <string>

#include "./Object.h"

namespace core {

    typedef std::shared_ptr<class Particle> ParticleRef;

    class Particle : public Object<Particle> {
    public:
        Particle();

        double density() { return density_; }
        Particle& density(double density);

        double pressure() { return pressure_; }
        Particle& pressure(double pressure);
    
    private:
        double density_, pressure_;

    };

};
