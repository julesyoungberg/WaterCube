#pragma once

#include <memory>
#include <string>

#include "cinder/gl/gl.h"

#include "./BaseObject.h"

using namespace ci;

namespace core {

    typedef std::shared_ptr<class Object> ObjectRef;

    class Object : public BaseObject {
    public:
        Object(const std::string& name);

        float mass() { return mass_; }
        Object mass(float mass);
        
        vec3 position() { return position_; }
        Object position(vec3 position);

        vec3 velocity() { return velocity_; }
        Object velocity(vec3 velocity);

        vec3 acceleration() { return acceleration_; }
        Object acceleration(vec3 acceleration);

        void applyForce(vec3 force);
        void update(float timestep);

        static ObjectRef create(const std::string& name);

    private:
        float mass_;
        vec3 position_, velocity_, acceleration_;

    };

};
