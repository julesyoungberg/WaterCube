#include <assert.h>

#include "cinder/Easing.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

#include "./core/Container.h"
#include "./core/Scene.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace core;

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

    scene_ = Scene::create();

    Container* container = new Container("container");
    scene_->addObject(BaseObjectRef(container));
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
