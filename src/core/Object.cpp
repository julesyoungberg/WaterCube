#include "./Object.h"

using namespace core;

template<class D> Object<D>::Object(const std::string& name):
    BaseObject(name),
    mass_(1.0),
    position_(0),
    velocity_(0),
    acceleration_(0)
{}

template<class D> void Object<D>::applyForce(vec3 force) {
    acceleration_ += force / mass_;
}

template<class D> void Object<D>::update(float time_step) {
    velocity_ += acceleration_ * time_step;
    position_ += velocity_ * time_step;
    acceleration_ *= 0.0;
}
