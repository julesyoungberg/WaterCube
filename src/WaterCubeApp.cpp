#include <assert.h>

#include "cinder/Easing.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

#include "./core/Container.h"
#include "./core/Fluid.h"
#include "./core/Scene.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace core;

const int NUM_PARTICLES = 1; // 10000;

// TODO plugins:
// https://github.com/simongeilfus/Watchdog

class WaterCubeApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;

    double prev_time_;
    CameraPersp cam_;
    SceneRef scene_;
};

void WaterCubeApp::setup() {
    prev_time_ = 0.0;
    cam_.lookAt(vec3(3, 2, 4), vec3(0));
    gl::enableDepthWrite();
    gl::enableDepthRead();

    console() << "creating scene\n";
    scene_ = Scene::create();

    ContainerRef container = Container::create("container");
    BaseObjectRef container_ref = std::dynamic_pointer_cast<BaseObject, Container>(container);
    assert(scene_->addObject(container_ref));

    FluidRef fluid = Fluid::create("fluid", container)
        ->numParticles(NUM_PARTICLES)
        ->setup();
    BaseObjectRef fluid_ref = std::dynamic_pointer_cast<BaseObject, Fluid>(fluid);
    assert(scene_->addObject(fluid_ref));

    console() << "done setup, created " << scene_->numObjects() << " objects\n";
}

void WaterCubeApp::update() {
    double time = getElapsedSeconds();
    scene_->update(time - prev_time_);
    prev_time_ = time;
}

void WaterCubeApp::draw() {
    gl::clear(Color(0.2f, 0.2f, 0.3f));
    gl::setMatrices(cam_);
    scene_->draw();
}

CINDER_APP(WaterCubeApp, RendererGl)
