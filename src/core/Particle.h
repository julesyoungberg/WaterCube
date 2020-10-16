#pragma once

#include <memory>
#include <string>

#include "./Object.h"

namespace core {

    typedef std::shared_ptr<class Particle> ParticleRef;

    class Particle : public Object {
    public:
        Particle();

        static ParticleRef create();
    };

};
