#include "cinder/app/App.h"

#include "./Container.h"

using namespace ci::app;
using namespace core;

Container::Container(const std::string& name, float size) : PhysicalObject<Container>(name), size_(size) {
    vertices_ = {
        vec3(0, 0, size_), vec3(size_, 0, size_), vec3(size_, size_, size_), vec3(0, size_, size_),
        vec3(0, 0, 0),     vec3(size_, 0, 0),     vec3(size_, size_, 0),     vec3(0, size_, 0),
    };

    box_indices_ = {0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 7, 6, 5, 5, 4, 7,
                    4, 0, 3, 3, 7, 4, 4, 5, 1, 1, 0, 4, 3, 2, 6, 6, 7, 3};

    edges_ = {ivec2(0, 1), ivec2(0, 3), ivec2(0, 4), ivec2(1, 2), ivec2(1, 5), ivec2(2, 3),
              ivec2(2, 6), ivec2(3, 7), ivec2(4, 5), ivec2(4, 7), ivec2(5, 6), ivec2(6, 7)};
}

void Container::setup() {}

void Container::update(double time) {}

void Container::draw() {
    for (auto edge : edges_) {
        vec3 a = vertices_[edge.x];
        vec3 b = vertices_[edge.y];
        gl::color(Color("black"));
        gl::drawLine(a, b);
    }
}

void Container::reset() {}

ContainerRef Container::create(const std::string& name, float size) { return std::make_shared<Container>(name, size); }
