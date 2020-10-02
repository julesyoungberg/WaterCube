#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class WaterCubeApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void WaterCubeApp::setup()
{
}

void WaterCubeApp::mouseDown( MouseEvent event )
{
}

void WaterCubeApp::update()
{
}

void WaterCubeApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( WaterCubeApp, RendererGl )
