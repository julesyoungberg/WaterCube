#pragma once

#include <memory>
#include <string>

#include "cinder/gl/gl.h"

#include "./Object.h"

using namespace ci;

namespace core {

typedef std::shared_ptr<class Container> ContainerRef;

class Container : public Object<Container> {
public:
    Container(const std::string& name);

    void update(double time) override;
    void draw() override;
    void reset() override;

    static ContainerRef create(const std::string& name);

private:
    std::vector<vec3> vertices_;
    std::vector<std::uint32_t> box_indices_;
    gl::BatchRef batch_;
    gl::GlslProgRef glsl_;

};

};
