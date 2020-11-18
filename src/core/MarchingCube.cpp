#include "MarchingCube.h"

#include <glm/gtx/string_cast.hpp>

#include "util.h"

using namespace core;

MarchingCube::MarchingCube() : threshold_(0.5f) {}

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

MarchingCubeRef MarchingCube::resolution(int r) {
    resolution_ = r;
    return thisRef();
}

/**
 * Prepare resulting triangle buffer
 */
void MarchingCube::prepareGridBuffer() {
    // The size of the buffer that holds the verts.
    // This is the maximum number of verts that the
    // marching cube can produce, 5 triangles for each voxel.
    count_ = resolution_ * resolution_ * resolution_ * (3 * 5);
    util::log("\tcreating grid buffer");
    grids_.resize(count_, Grid());
    grid_buffer_ = gl::Ssbo::create(grids_.size() * sizeof(Grid), grids_.data(), GL_STATIC_DRAW);

    util::log("\tcreating ids vbo");
    std::vector<GLuint> ids(grids_.size());
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    grid_ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating attributes vao");
    grid_attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedIds(grid_ids_vbo_);
    gl::ScopedVao vao(grid_attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
}

/**
 * prepare debug particles
 */
void MarchingCube::prepareDensityParticles() {
    util::log("\tcreating density particles");
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

    prepareGridBuffer();

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

    util::log("\tcreating density buffer");
    std::vector<float> zeros(int(pow(resolution_ + 1, 3)), 0);
    density_buffer_ = gl::Ssbo::create(zeros.size() * sizeof(float), zeros.data(), GL_STATIC_DRAW);

    prepareDensityParticles();
}

/**
 * compile shader programs
 */
void MarchingCube::compileShaders() {
    util::log("compiling marching cube shaders");

    util::log("\tcompiling bin density prog");
    bin_density_prog_ = util::compileComputeShader("marchingCube/binDensity.comp");

    util::log("\tcompiling marching cube prog");
    marching_cube_prog_ = util::compileComputeShader("marchingCube/marchingCube.comp");

    util::log("\tcompiling clear prog");
    clear_prog_ = util::compileComputeShader("marchingCube/clear.comp");

    util::log("\tcompiling render density prog");
    render_density_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("marchingCube/density.vert"))
                                 .fragment(loadAsset("marchingCube/density.frag"))
                                 .attribLocation("particleID", 0));

    util::log("\tcompiling render grid prog");
    render_grid_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                                 .vertex(loadAsset("marchingCube/grid.vert"))
                                                 .fragment(loadAsset("marchingCube/grid.frag"))
                                                 .attribLocation("gridID", 0));

    util::log("\tcompiling render surface prog");
    render_surface_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("marchingCube/surface.vert"))
                                 .fragment(loadAsset("marchingCube/surface.frag"))
                                 .attribLocation("gridID", 0));
}

/**
 * setup given a grid resolution
 */
void MarchingCube::setup() {
    prepareBuffers();
    compileShaders();
    clearDensity();
}

/**
 * run clear program to clear the grid
 */
void MarchingCube::runClearProg() {
    const auto thread = count_ / CLEAR_GROUP_SIZE;
    grid_buffer_->bindBase(0);

    gl::ScopedGlslProg prog(clear_prog_);
    clear_prog_->uniform("count", count_);
    util::runProg(thread);
}

/**
 * clear density buffer
 */
void MarchingCube::clearDensity() {
    std::vector<uint32_t> zeros(int(pow(resolution_, 3)), 0);
    gl::ScopedBuffer buffer(density_buffer_);
    glClearBufferData(density_buffer_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                      zeros.data());
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

/**
 * compute density for points on the grid
 */
void MarchingCube::runBinDensityProg(GLuint particle_buffer, GLuint count_buffer,
                                     GLuint offset_bufer) {
    gl::ScopedGlslProg prog(bin_density_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);

    gl::ScopedBuffer density_buffer(density_buffer_);
    density_buffer_->bindBase(1);

    bin_density_prog_->uniform("size", size_);
    bin_density_prog_->uniform("res", resolution_);

    util::runProg(ivec3(int(ceil(float(resolution_ + 1) / 4.0f))));
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * run main marching cube algorithm
 */
void MarchingCube::runMarchingCubeProg(const ivec3 thread) {
    gl::ScopedGlslProg prog(marching_cube_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer scoped_grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_flags_buffer(cube_edge_flags_buffer_);
    cube_edge_flags_buffer_->bindBase(1);

    gl::ScopedBuffer scoped_table_buffer(triangle_connection_table_buffer_);
    triangle_connection_table_buffer_->bindBase(2);

    gl::ScopedBuffer scoped_density(density_buffer_);
    density_buffer_->bindBase(3);

    marching_cube_prog_->uniform("size", size_);
    marching_cube_prog_->uniform("res", resolution_);
    // marching_cube_prog_->uniform("border", 1);
    marching_cube_prog_->uniform("threshold", threshold_);

    util::runProg(thread);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * prints density buffer for debugging
 */
void MarchingCube::printDensity() {
    auto densities = util::getFloats(density_buffer_->getId(), int(pow(resolution_, 3)));
    std::string s = "densities: ";
    for (int i = 0; i < 100; i++) {
        s += std::to_string(densities[i]) + ", ";
    }
    util::log("%s", s.c_str());
}

/**
 * prints grid buffer for debugging
 */
void MarchingCube::printGrid() {
    std::vector<Grid> grid(count_);
    gl::ScopedBuffer scoped_buffer(grid_buffer_);

    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count_ * sizeof(Grid), grid.data());
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

    std::string s = "grid: ";
    for (int i = 0; i < 10; i++) {
        auto p = grid[i].position;
        auto n = grid[i].normal;
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
    const ivec3 thread = ivec3(int(ceil(resolution_ / MARCHING_CUBE_GROUP_SIZE)));

    runClearProg();
    clearDensity();

    runBinDensityProg(particle_buffer, count_buffer, offset_buffer);
    runMarchingCubeProg(thread);
    // printGrid();
}

/**
 * render debug density grid
 */
void MarchingCube::renderDensity() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_density_prog_);
    gl::ScopedVao vao(particle_attributes_);

    gl::ScopedBuffer particle_buffer(particle_buffer_);
    particle_buffer_->bindBase(0);

    gl::ScopedBuffer density_buffer(density_buffer_);
    density_buffer_->bindBase(1);

    render_density_prog_->uniform("size", size_);
    render_density_prog_->uniform("res", resolution_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, int(particles_.size()));
}

/**
 * render debugging grid
 */
void MarchingCube::renderGrid() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_grid_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, count_);
}

/**
 * render resulting surface
 */
void MarchingCube::renderSurface() {
    gl::ScopedDepthTest depthTest(true);

    gl::ScopedGlslProg render(render_surface_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    gl::context()->setDefaultShaderVars();
    gl::drawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
}
