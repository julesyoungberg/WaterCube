#pragma once

#include <vector>

#include "cinder/app/App.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/gl.h"

#include "./marching_cube_lookup.h"
#include "./util.h"

#define MARCHING_CUBE_GROUP_SIZE 8
#define CLEAR_GROUP_SIZE 1024

using namespace ci;
using namespace ci::app;

struct Vertex {
    vec4 position;
    vec4 normal;
};

typedef std::shared_ptr<class MarchingCube> MarchingCubeRef;

class MarchingCube {
public:
    MarchingCube();
    ~MarchingCube();

    MarchingCubeRef numItems(int n);
    MarchingCubeRef sortingResolution(int r);
    MarchingCubeRef subdivisions(int s);
    MarchingCubeRef size(float s);
    MarchingCubeRef threshold(float t);
    MarchingCubeRef cameraPosition(vec3 p);

    void setup();
    void update(GLuint particle_buffer, GLuint count_buffer, GLuint offset_buffer);
    void renderParticleGrid();
    void renderSurfaceVertices();
    void renderSurface();

    void updateField(float time, float scale);

    static MarchingCubeRef create() { return std::make_shared<MarchingCube>(); }

private:
    void prepareVertexBuffer();
    void prepareDebugParticles();
    void prepareBuffers();
    void compileShaders();

    void clearDensity();
    void clearPressure();

    void runClearProg();
    void runDiscretizeProg(GLuint particle_buffer, GLuint count_buffer, GLuint offset_buffer);
    void runMarchingCubeProg();
    void printDensity();
    void printPressure();
    void printVertices();

    MarchingCubeRef thisRef() { return std::make_shared<MarchingCube>(*this); }

    int count_;
    int sorting_resolution_;
    int subdivisions_;
    int resolution_;
    int num_items_;
    float size_;
    float threshold_;

    vec3 light_position_, camera_position_;

    gl::GlslProgRef discretize_prog_;
    gl::GlslProgRef marching_cube_prog_, clear_prog_;
    gl::GlslProgRef render_particle_grid_prog_;
    gl::GlslProgRef render_vertices_prog_;
    gl::GlslProgRef render_surface_prog_;

    std::vector<Vertex> vertices_;
    std::vector<int> indices_;
    std::vector<ivec4> particles_;

    gl::SsboRef vertex_buffer_, index_buffer_;
    gl::SsboRef particle_buffer_, density_buffer_;
    gl::SsboRef cube_edge_flags_buffer_, triangle_connection_table_buffer_;
    gl::Texture3dRef pressure_field_;
    gl::VboRef vertex_ids_vbo_, particle_ids_vbo_;
    gl::VaoRef vertex_attributes_, particle_attributes_;
};
