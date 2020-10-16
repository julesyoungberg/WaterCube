#pragma once

#include <memory>
#include <string>

#include "cinder/gl/gl.h"

#include "./Object.h"

using namespace ci;

namespace core {

    typedef std::shared_ptr<class Container> ContainerRef;

    class Container : public Object {
    public:
        Container(const std::string& name);

        void draw();

        static ContainerRef create(const std::string& name);

    private:
        gl::BatchRef batch_;
        gl::GlslProgRef glsl_;

    };

};
