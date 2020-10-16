#pragma once

#include <memory>
#include <string>

#include "./BaseObject.h"
#include "./Container.h"
#include "./Particle.h"

namespace core {

    typedef std::shared_ptr<class Fluid> FluidRef;

    class Fluid : public BaseObject {
    public:
        Fluid(const std::string& name);
        ~Fluid();

        int numParticles() { return num_particles_; }
        Fluid numParticles(int n) { num_particles_ = n; return *this; }

    private:
        int num_particles_;
        std::vector<Particle> particles_;
    
    };

};
