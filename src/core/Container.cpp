#include "cinder/app/App.h"

#include "./Container.h"

using namespace ci::app;
using namespace core;

Container::Container(const std::string& name): Object<Container>(name) {
    glsl_ = gl::GlslProg::create(loadAsset("shaders/container.vert"),
                                 loadAsset("shaders/container.frag"));

    batch_ = gl::Batch::create(geom::Cube(), glsl_);
}

void Container::update(double time) {

}

void Container::draw() {
    batch_->draw();
}

void Container::reset() {

}

ContainerRef Container::create(const std::string& name) {
    return ContainerRef(new Container(name));
}
