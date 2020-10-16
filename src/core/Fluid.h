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
        std::vector<Particle> particles_;
        ContainerRef container_;
    
    };

};
