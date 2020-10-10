#include "cinder/Easing.h"
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// TODO plugins:
// https://github.com/simongeilfus/Watchdog

class WaterCubeApp : public App {
public:
    void setup() override;
    void update() override;
    void draw() override;

    static const int NUM_SLICES = 12;

    CameraPersp cam_;
    gl::BatchRef cube_;
    gl::GlslProgRef glsl_;
};

void WaterCubeApp::setup() {
    cam_.lookAt(vec3(3, 2, 4), vec3(0));

    glsl_ = gl::GlslProg::create(loadAsset("shaders/container.vert"),
                                 loadAsset("shaders/container.frag"));

    cube_ = gl::Batch::create(geom::Cube(), glsl_);

    gl::enableDepthWrite();
    gl::enableDepthRead();
}

void WaterCubeApp::update() {}

void WaterCubeApp::draw() {
    gl::clear();
    gl::setMatrices(cam_);
    cube_->draw();
}

CINDER_APP(WaterCubeApp, RendererGl)
