#pragma once

#include <Windows.h>
#include <memory>
#include <string>

#include "cinder/gl/gl.h"

#include "./PhysicalObject.h"
#include "./util.h"

using namespace ci;

namespace core {

typedef std::shared_ptr<class Container> ContainerRef;

/**
 * Class representing water container
 */
class Container : public PhysicalObject<Container> {
public:
    Container(const std::string& name, float size);

    void update(double time) override;
    void draw() override;
    void reset() override;

    static ContainerRef create(const std::string& name, float size);

private:
    float size_;
    std::vector<vec3> vertices_;
    std::vector<std::uint32_t> box_indices_;
    std::vector<ivec2> edges_;
    gl::BatchRef batch_;
    gl::GlslProgRef render_prog_;
};

} // namespace core
