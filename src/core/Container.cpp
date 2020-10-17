#include "cinder/app/App.h"

#include "./Container.h"

using namespace ci::app;
using namespace core;

Container::Container(const std::string& name)
    : PhysicalObject<Container>(name) {
    glsl_ = gl::GlslProg::create(loadAsset("container.vert"),
                                 loadAsset("container.frag"));

    auto geometry = geom::Cube();
    batch_ = gl::Batch::create(geometry, glsl_);

    vertices_ = {
        vec3(0, 0, scale_.z),
        vec3(scale_.x, 0, scale_.z),
        vec3(scale_.x, scale_.y, scale_.z),
        vec3(0, scale_.y, scale_.z),
        vec3(0, 0, 0),
        vec3(scale_.x, 0, 0),
        vec3(scale_.x, scale_.y, 0),
        vec3(0, scale_.y, 0),
    };

    box_indices_ = {0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7,
                    4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3};
}

void Container::update(double time) {}

void Container::draw() {
    // batch_->draw();
}

void Container::reset() {}

ContainerRef Container::create(const std::string& name) {
    return std::make_shared<Container>(name);
}
