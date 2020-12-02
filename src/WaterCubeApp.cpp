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

class WaterCubeApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;
    void keyDown(KeyEvent event) override;
    void mouseMove(MouseEvent event) override;

private:
    Ray getMouseRay();

    bool run_once_, running_, reset_;
    double prev_time_;
    float size_;

    ivec2 mouse_position_;

    CameraPersp cam_;
    SceneRef scene_;
    FluidRef fluid_;
};

void WaterCubeApp::setup() {
    run_once_ = false;
    running_ = false;
    reset_ = false;
    size_ = 1.0;
    prev_time_ = 0.0f;

    cam_.setPerspective(45.0f, getWindowAspectRatio(), 0.1f, 1000.0f);
    vec3 camera_pos = vec3(-size_, size_ / 3.0f, size_ * 2);
    cam_.lookAt(camera_pos, vec3(0, 0, 0));

    gl::enableDepthWrite();
    gl::enableDepthRead();

    util::log("creating scene");
    scene_ = Scene::create();

    fluid_ = Fluid::create("fluid");
    fluid_->setup();
    fluid_->setCameraPosition(camera_pos);
    fluid_->setLightPosition(vec3(0, size_ / 2.0f, size_));

    BaseObjectRef fluid_ref = std::dynamic_pointer_cast<BaseObject, Fluid>(fluid_);
    CI_ASSERT(scene_->addObject(fluid_ref));
}

Ray WaterCubeApp::getMouseRay() {
    // Generate a ray from the camera into our world. Note that we have to
    // flip the vertical coordinate.
    float u = mouse_position_.x / (float)getWindowWidth();
    float v = mouse_position_.y / (float)getWindowHeight();
    Ray ray = cam_.generateRay(u, 1.0f - v, cam_.getAspectRatio());
    return ray;
}

void WaterCubeApp::update() {
    double time = getElapsedSeconds();
    double step = time - prev_time_;
    prev_time_ = time;

    if (!running_) {
        if (reset_) {
            setup();
            running_ = true;
            reset_ = false;
        }

        return;
    }

    if (step > 1) {
        // better luck next time
        return;
    }

    fluid_->setMouseRay(getMouseRay());

    scene_->update(step);

    if (run_once_) {
        running_ = false;
    }
}

void WaterCubeApp::draw() {
    gl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::clear(Color(0.8f, 0.8f, 0.8f));
    gl::setMatricesWindowPersp(getWindowSize());
    gl::setMatrices(cam_);

    scene_->draw();

    vec2 window = app::getWindowSize();
    gl::setMatricesWindow(window);
    std::string font = "Ariel";
    gl::drawString(toString(static_cast<int>(getAverageFps())) + " fps",
                   vec2(window.x - 64.0f, 20.0f), Color("black"), Font(font, 20));
}

void WaterCubeApp::keyDown(KeyEvent event) {
    char c = event.getChar();
    switch (c) {
    case 's':
        running_ = !running_;
    case 'r':
        running_ = false;
        reset_ = true;
    }
}

void WaterCubeApp::mouseMove(MouseEvent event) { mouse_position_ = event.getPos(); }

CINDER_APP(WaterCubeApp, RendererGl,
           [](App::Settings* settings) { settings->setWindowSize(1920, 1080); })
