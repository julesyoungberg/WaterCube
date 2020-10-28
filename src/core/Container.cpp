#include "cinder/app/App.h"

#include "./Container.h"

using namespace ci::app;
using namespace core;

Container::Container(const std::string& name, float size) : PhysicalObject<Container>(name), size_(size) {
    render_prog_ = gl::GlslProg::create(loadAsset("container.vert"), loadAsset("container.frag"));

    auto geometry = geom::Cube();
    batch_ = gl::Batch::create(geometry, render_prog_);

    vertices_ = {
        vec3(0, 0, size_), vec3(size_, 0, size_), vec3(size_, size_, size_), vec3(0, size_, size_),
        vec3(0, 0, 0),     vec3(size_, 0, 0),     vec3(size_, size_, 0),     vec3(0, size_, 0),
    };

    box_indices_ = {0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7,
                    4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3};
}

void Container::update(double time) {}

void Container::draw() {
    // batch_->draw();
}

void Container::reset() {}

ContainerRef Container::create(const std::string& name, float size) { return std::make_shared<Container>(name, size); }
