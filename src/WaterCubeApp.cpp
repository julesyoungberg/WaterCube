#include <Windows.h>
#include <string>

#include "cinder/Easing.h"
#include "cinder/Utilities.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "./core/Fluid.h"
#include "./core/Scene.h"

using namespace std;
using namespace ci;
using namespace ci::app;
using namespace core;

const int NUM_PARTICLES = static_cast<int>(20e3);

class WaterCubeApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyDown(KeyEvent event) override;

    bool run_once_, running_, reset_;
    double prev_time_;
    float size_;
    CameraPersp cam_;
    SceneRef scene_;
    params::InterfaceGlRef params_;
    quat scene_rotation_;
    FluidRef fluid_;
};

void WaterCubeApp::setup() {
    run_once_ = false;
    running_ = true;
    reset_ = false;
    size_ = 1.0f;
    prev_time_ = 0.0;

    params_ = params::InterfaceGl::create("WaterCube", ivec2(225, 200));
    params_->addParam("Scene Rotation", &scene_rotation_);

    cam_.setPerspective(45.0f, getWindowAspectRatio(), 0.1f, 1000.0f);
    vec3 camera_pos = vec3(size_, size_, size_ * 3) / 1.5f;
    cam_.lookAt(camera_pos, vec3(0, 0, 0));

    gl::enableDepthWrite();
    gl::enableDepthRead();

    util::log("creating scene");
    scene_ = Scene::create();

    fluid_ = Fluid::create("fluid")
                 ->size(size_)
                 ->numParticles(NUM_PARTICLES)
                 ->cameraPosition(camera_pos);
    fluid_->addParams(params_);
    fluid_->setup();

    BaseObjectRef fluid_ref = std::dynamic_pointer_cast<BaseObject, Fluid>(fluid_);
    CI_ASSERT(scene_->addObject(fluid_ref));
}

void WaterCubeApp::update() {
    if (!running_) {
        if (reset_) {
            setup();
            running_ = true;
            reset_ = false;
        }

        return;
    }

    fluid_->setRotation(scene_rotation_);

    double time = getElapsedSeconds();
    scene_->update(time - prev_time_);
    prev_time_ = time;

    if (run_once_) {
        running_ = false;
    }
}

void WaterCubeApp::draw() {
    gl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::clear(Color(0.3f, 0.3f, 0.35f));
    gl::setMatricesWindowPersp(getWindowSize());
    gl::setMatrices(cam_);
    gl::rotate(scene_rotation_);

    scene_->draw();
    params_->draw();

    vec2 window = app::getWindowSize();
    gl::setMatricesWindow(window);
    gl::drawString(toString(static_cast<int>(getAverageFps())) + " fps",
                   vec2(window.x - 64.0f, 100.0f));
}

void WaterCubeApp::keyDown(KeyEvent event) {
    char c = event.getChar();
    switch (c) {
    case 'p':
        running_ = !running_;
    case 'r':
        running_ = false;
        reset_ = true;
    }
}

CINDER_APP(WaterCubeApp, RendererGl,
           [](App::Settings* settings) { settings->setWindowSize(1920, 1080); })
