#include "MarchingCube.h"

#include <glm/gtx/string_cast.hpp>

#include "util.h"

using namespace core;

MarchingCube::MarchingCube()
    : threshold_(0.5f), resolution_(0), sorting_resolution_(0), subdivisions_(0), num_items_(0),
      size_(0), light_position_(0), camera_position_(0) {}

MarchingCube::~MarchingCube() {}

MarchingCubeRef MarchingCube::size(float s) {
    size_ = s;
    return thisRef();
}

MarchingCubeRef MarchingCube::numItems(int n) {
    num_items_ = n;
    return thisRef();
}

MarchingCubeRef MarchingCube::threshold(float t) {
    threshold_ = t;
    return thisRef();
}

MarchingCubeRef MarchingCube::sortingResolution(int r) {
    sorting_resolution_ = r;
    return thisRef();
}

MarchingCubeRef MarchingCube::subdivisions(int s) {
    subdivisions_ = s;
    return thisRef();
}

MarchingCubeRef MarchingCube::cameraPosition(vec3 p) {
    camera_position_ = p;
    return thisRef();
}

/**
 * Prepare resulting triangle buffer
 */
void MarchingCube::prepareVertexBuffer() {
    // The size of the buffer that holds the verts.
    // This is the maximum number of verts that the
    // marching cube can produce, 5 triangles for each voxel.
    count_ = resolution_ * resolution_ * resolution_ * (3 * 5);
    util::log("\tcreating grid buffer");
    vertices_.resize(count_, Vertex());
    vertex_buffer_ =
        gl::Ssbo::create(vertices_.size() * sizeof(Vertex), vertices_.data(), GL_STATIC_DRAW);

    util::log("\tcreating ids vbo");
    std::vector<GLuint> ids(vertices_.size());
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    vertex_ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating attributes vao");
    vertex_attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedIds(vertex_ids_vbo_);
    gl::ScopedVao vao(vertex_attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
}

/**
 * prepare debug particles
 */
void MarchingCube::prepareDebugParticles() {
    util::log("\tcreating debug particles");
    particles_.resize(int(pow(resolution_, 3)));
    for (int z = 0; z < resolution_; z++) {
        for (int y = 0; y < resolution_; y++) {
            for (int x = 0; x < resolution_; x++) {
                particles_.push_back(ivec4(x, y, z, 0));
            }
        }
    }
    particle_buffer_ =
        gl::Ssbo::create(particles_.size() * sizeof(ivec4), particles_.data(), GL_STATIC_DRAW);

    util::log("\tcreating density particles ids vbo");
    std::vector<GLuint> dids(particles_.size());
    GLuint curr_id = 0;
    std::generate(dids.begin(), dids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    particle_ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, dids, GL_STATIC_DRAW);

    util::log("\tcreating density attributes vao");
    particle_attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedDids(particle_ids_vbo_);
    gl::ScopedVao particle_vao(particle_attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
}

/**
 * create GPU buffers with initial data
 */
void MarchingCube::prepareBuffers() {
    util::log("preparing marching cube buffers");

    prepareVertexBuffer();

    util::log("\tcreating cube edge flags buffer");
    auto flags = std::vector<int>(256);
    for (int i = 0; i < 256; i++) {
        flags[i] = cubeEdgeFlags[i];
    }
    cube_edge_flags_buffer_ =
        gl::Ssbo::create(flags.size() * sizeof(int), flags.data(), GL_STATIC_DRAW);

    util::log("\tcreating triangle connection table buffer");
    auto connection_table = std::vector<int>();
    for (int y = 0; y < 256; y++) {
        auto row = triangleConnectionTable[y];
        for (int x = 0; x < 16; x++) {
            connection_table.push_back(row[x]);
        }
    }
    triangle_connection_table_buffer_ = gl::Ssbo::create(connection_table.size() * sizeof(int),
                                                         connection_table.data(), GL_STATIC_DRAW);

    util::log("\tcreating density and pressure buffers");
    std::vector<float> zeros(int(pow(resolution_ + 1, 3)), 0);
    density_buffer_ = gl::Ssbo::create(zeros.size() * sizeof(float), zeros.data(), GL_STATIC_DRAW);
    clearPressure(); // initializes the texture

    prepareDebugParticles();
}

/**
 * compile shader programs
 */
void MarchingCube::compileShaders() {
    util::log("compiling marching cube shaders");

    util::log("\tcompiling discretize prog");
    discretize_prog_ = util::compileComputeShader("marchingCube/discretize.comp");

    util::log("\tcompiling marching cube prog");
    marching_cube_prog_ = util::compileComputeShader("marchingCube/marchingCube.comp");

    util::log("\tcompiling clear prog");
    clear_prog_ = util::compileComputeShader("marchingCube/clear.comp");

    util::log("\tcompiling render particle grid prog");
    render_particle_grid_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("marchingCube/particleGrid.vert"))
                                 .fragment(loadAsset("marchingCube/particleGrid.frag"))
                                 .attribLocation("particleID", 0));

    util::log("\tcompiling render surface points prog");
    render_vertices_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("marchingCube/surfaceVertex.vert"))
                                 .fragment(loadAsset("marchingCube/surfaceVertex.frag"))
                                 .attribLocation("vertexID", 0));

    util::log("\tcompiling render surface prog");
    render_surface_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("marchingCube/surface.vert"))
                                 .fragment(loadAsset("marchingCube/surface.frag"))
                                 .attribLocation("vertexID", 0));
}

/**
 * setup given a grid resolution
 */
void MarchingCube::setup() {
    resolution_ = sorting_resolution_ * subdivisions_;
    light_position_ = vec3(size_ / 2.0f, size_ * 2.0f, size_ / 2.0f);
    prepareBuffers();
    compileShaders();
}

/**
 * run clear program to clear the grid
 */
void MarchingCube::runClearProg() {
    vertex_buffer_->bindBase(0);
    gl::ScopedGlslProg prog(clear_prog_);
    clear_prog_->uniform("count", count_);
    util::runProg(int(ceil(float(count_) / CLEAR_GROUP_SIZE)));
}

/**
 * clear density buffer
 */
void MarchingCube::clearDensity() {
    std::vector<float> zeros(int(pow(resolution_ + 1, 3)), 0);
    gl::ScopedBuffer buffer(density_buffer_);
    glClearBufferData(density_buffer_->getTarget(), GL_R32F, GL_RED, GL_FLOAT, zeros.data());
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

/**
 * clear pressure buffer
 */
void MarchingCube::clearPressure() {
    auto format = gl::Texture3d::Format().internalFormat(GL_R32F);
    std::vector<std::uint32_t> data(int(pow(resolution_ + 1, 3)), 0);
    pressure_field_ = gl::Texture3d::create(data.data(), GL_RED, resolution_ + 1, resolution_ + 1,
                                            resolution_ + 1, format);
}

/**
 * compute density and pressure for points on the grid
 */
void MarchingCube::runDiscretizeProg(GLuint particle_buffer, GLuint count_buffer,
                                     GLuint offset_buffer) {
    gl::ScopedGlslProg prog(discretize_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, count_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, offset_buffer);

    gl::ScopedBuffer density_buffer(density_buffer_);
    density_buffer_->bindBase(3);

    glActiveTexture(GL_TEXTURE0 + 4);
    glUniform1i(glGetUniformLocation(discretize_prog_->getHandle(), "pressureField"), 4);
    glBindImageTexture(4, pressure_field_->getId(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);

    discretize_prog_->uniform("pressureField", 4);
    discretize_prog_->uniform("size", size_);
    discretize_prog_->uniform("sortRes", sorting_resolution_);
    discretize_prog_->uniform("res", resolution_);

    util::runProg(ivec3(int(ceil(float(resolution_ + 1) / MARCHING_CUBE_GROUP_SIZE))));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * run main marching cube algorithm
 */
void MarchingCube::runMarchingCubeProg() {
    gl::ScopedGlslProg prog(marching_cube_prog_);
    gl::ScopedVao vao(vertex_attributes_);

    gl::ScopedBuffer vertex_buffer(vertex_buffer_);
    vertex_buffer_->bindBase(0);

    gl::ScopedBuffer flags_buffer(cube_edge_flags_buffer_);
    cube_edge_flags_buffer_->bindBase(1);

    gl::ScopedBuffer table_buffer(triangle_connection_table_buffer_);
    triangle_connection_table_buffer_->bindBase(2);

    gl::ScopedBuffer scoped_density(density_buffer_);
    density_buffer_->bindBase(3);

    marching_cube_prog_->uniform("size", size_);
    marching_cube_prog_->uniform("res", resolution_);
    marching_cube_prog_->uniform("threshold", threshold_);

    util::runProg(ivec3(int(ceil(float(resolution_) / MARCHING_CUBE_GROUP_SIZE))));
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * prints density buffer for debugging
 */
void MarchingCube::printDensity() {
    auto densities = util::getFloats(density_buffer_->getId(), int(pow(resolution_ + 1, 3)));
    std::string s = "densities: ";
    for (int i = 0; i < 100; i++) {
        s += std::to_string(densities[i]) + ", ";
    }
    util::log("%s", s.c_str());
}

/**
 * prints pressure buffer for debugging
 */
void MarchingCube::printPressure() {
    auto values = util::getFloats(pressure_field_, int(pow(resolution_ + 1, 3)));
    std::string s = "pressures: ";
    for (int i = 0; i < 100; i++) {
        s += std::to_string(values[i]) + ", ";
    }
    util::log("%s", s.c_str());
}

/**
 * prints vertex buffer for debugging
 */
void MarchingCube::printVertices() {
    std::vector<Vertex> vertices(count_);
    gl::ScopedBuffer scoped_buffer(vertex_buffer_);

    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count_ * sizeof(Vertex), vertices.data());
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    std::string s = "vertices: ";
    for (int i = 0; i < 10; i++) {
        auto p = vertices[i].position;
        auto n = vertices[i].normal;
        std::string ps = glm::to_string(p);
        std::string ns = glm::to_string(n);
        s += "(p=<" + ps + ">, n=<" + ns + ">), ";
    }
    util::log("%s", s.c_str());
}

/**
 * update routine
 */
void MarchingCube::update(GLuint particle_buffer, GLuint count_buffer, GLuint offset_buffer) {
    runClearProg();
    clearDensity();
    clearPressure();

    runDiscretizeProg(particle_buffer, count_buffer, offset_buffer);
    // printDensity();
    // printPressure();

    runMarchingCubeProg();
    // printVertices();
}

/**
 * render debug particle grid
 */
void MarchingCube::renderParticleGrid() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_particle_grid_prog_);
    gl::ScopedVao vao(particle_attributes_);

    gl::ScopedBuffer particle_buffer(particle_buffer_);
    particle_buffer_->bindBase(0);

    gl::ScopedBuffer density_buffer(density_buffer_);
    density_buffer_->bindBase(1);

    pressure_field_->bind(2);

    render_particle_grid_prog_->uniform("pressureField", 2);
    render_particle_grid_prog_->uniform("size", size_);
    render_particle_grid_prog_->uniform("res", resolution_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, int(particles_.size()));
}

/**
 * render surface points for debugging
 */
void MarchingCube::renderSurfaceVertices() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_vertices_prog_);
    gl::ScopedVao vao(vertex_attributes_);

    gl::ScopedBuffer vertex_buffer(vertex_buffer_);
    vertex_buffer_->bindBase(0);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, count_);
}

/**
 * render resulting surface
 */
void MarchingCube::renderSurface() {
    gl::ScopedDepthTest depthTest(true);

    gl::ScopedGlslProg render(render_surface_prog_);
    gl::ScopedVao vao(vertex_attributes_);

    gl::ScopedBuffer vertex_buffer(vertex_buffer_);
    vertex_buffer_->bindBase(0);

    pressure_field_->bind(1);

    render_surface_prog_->uniform("pressureField", 1);
    render_surface_prog_->uniform("size", size_);
    render_surface_prog_->uniform("lightPos", light_position_);
    render_surface_prog_->uniform("cameraPos", camera_position_);

    gl::context()->setDefaultShaderVars();
    // gl::drawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
    gl::drawArrays(GL_TRIANGLES, 0, count_);
}
