#include <Windows.h>
#include <string>

#include "cinder/Easing.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

#include "./core/Fluid.h"
#include "./core/Scene.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace core;

const int NUM_PARTICLES = static_cast<int>(100e3);

class WaterCubeApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;

    bool run_once_, running_;
    double prev_time_;
    float size_;
    CameraPersp cam_;
    SceneRef scene_;
};

void WaterCubeApp::setup() {
    run_once_ = false;
    running_ = true;
    size_ = 5.0f;
    prev_time_ = 0.0;
    cam_.lookAt(vec3(size_, size_ * 2, size_ * 4), vec3(0, 0, 0));

    gl::enableDepthWrite();
    gl::enableDepthRead();

    util::log("creating scene");
    scene_ = Scene::create();

    FluidRef fluid = Fluid::create("fluid")->size(size_)->numParticles(NUM_PARTICLES)->setup();
    BaseObjectRef fluid_ref = std::dynamic_pointer_cast<BaseObject, Fluid>(fluid);
    CI_ASSERT(scene_->addObject(fluid_ref));
}

void WaterCubeApp::update() {
    if (!running_) {
        return;
    }

    double time = getElapsedSeconds();
    scene_->update(time - prev_time_);
    prev_time_ = time;
}

void WaterCubeApp::draw() {
    if (!running_) {
        return;
    }

    gl::clear(Color(0.2f, 0.2f, 0.3f));
    gl::setMatricesWindowPersp(getWindowSize());
    gl::setMatrices(cam_);

    scene_->draw();

    gl::setMatricesWindow(app::getWindowSize());
    gl::drawString(toString(static_cast<int>(getAverageFps())) + " fps", vec2(64.0f, 100.0f));

    if (run_once_) {
        running_ = false;
    }
}

CINDER_APP(WaterCubeApp, RendererGl, [](App::Settings* settings) { settings->setWindowSize(1920, 1080); })
