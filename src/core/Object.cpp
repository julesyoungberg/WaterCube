#include "./Object.h"

using namespace core;

Object::Object(const std::string& name):
    BaseObject(name),
    mass_(1.0),
    position_(0),
    velocity_(0),
    acceleration_(0)
{}

Object Object::mass(float mass) {
    mass_ = mass;
    return *this;
}

Object Object::position(vec3 position) {
    position_ = position;
    return *this;
}

Object Object::velocity(vec3 velocity) {
    velocity_ = velocity;
    return *this;
}

Object Object::acceleration(vec3 acceleration) {
    acceleration_ = acceleration;
    return *this;
}

void Object::applyForce(vec3 force) {
    acceleration_ += force / mass_;
}

void Object::update(float time_step) {
    velocity_ += acceleration_ * time_step;
    position_ += velocity_ * time_step;
    acceleration_ *= 0.0;
}

ObjectRef Object::create(const std::string& name) {
    return ObjectRef(new Object(name));
}
