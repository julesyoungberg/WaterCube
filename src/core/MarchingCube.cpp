#include "MarchingCube.h"

using namespace core;

MarchingCube::MarchingCube() {}

MarchingCube::~MarchingCube() {}

MarchingCubeRef MarchingCube::size(float s) {
    size_ = s;
    return thisRef();
}

void MarchingCube::prepareBuffers() {
    util::log("preparing marching cube buffers");
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
    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating attributes vao");
    attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedIds(ids_vbo_);
    gl::ScopedVao vao(attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

    util::log("\tcreating cube edge flags buffer");
    auto flags = std::vector<int>(256);
    for (int i = 0; i < 256; i++) {
        flags[i] = cubeEdgeFlags[i];
    }
    cube_edge_flags_buffer_ = gl::Ssbo::create(flags.size() * sizeof(int), flags.data(), GL_STATIC_DRAW);

    util::log("\tcreating triangle connection table buffer");
    auto connection_table = std::vector<int>();
    for (int y = 0; y < 256; y++) {
        auto row = triangleConnectionTable[y];
        for (int x = 0; x < 16; x++) {
            connection_table.push_back(row[x]);
        }
    }
    triangle_connection_table_buffer_ =
        gl::Ssbo::create(connection_table.size() * sizeof(int), connection_table.data(), GL_STATIC_DRAW);

    util::log("\tcreating density field");
    auto density_format = gl::Texture3d::Format().internalFormat(GL_R32UI);
    density_field_ = gl::Texture3d::create(resolution_, resolution_, resolution_, density_format);
}

void MarchingCube::compileShaders() {
    util::log("compiling marching cube shaders");

    util::log("\tcompiling bin density prog");
    bin_density_prog_ = util::compileComputeShader("marchingCube/binDensity.comp");

    util::log("\tcompiling normalize density prog");
    normalize_density_prog_ = util::compileComputeShader("marchingCube/normalizeDensity.comp");

    util::log("\tcompiling marching cube prog");
    marching_cube_prog_ = util::compileComputeShader("marchingCube/marchingCube.comp");

    util::log("\tcompiling clear prog");
    clear_prog_ = util::compileComputeShader("marchingCube/clear.comp");

    util::log("\tcompiling render prog");
    render_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                            .vertex(loadAsset("marchingCube/grid.vert"))
                                            .fragment(loadAsset("marchingCube/grid.frag"))
                                            .attribLocation("gridId", 0));
}

void MarchingCube::setup(const int resolution) {
    resolution_ = resolution;
    prepareBuffers();
    compileShaders();
}

void MarchingCube::runClearProg() {
    const auto thread = count_ / CLEAR_GROUP_SIZE;
    grid_buffer_->bindBase(8);

    gl::ScopedGlslProg prog(clear_prog_);
    clear_prog_->uniform("count", count_);
    util::runProg(thread);
}

void MarchingCube::clearDensity() {
    const std::uint32_t clear_value = 0;
    glClearTexImage(density_field_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
}

void MarchingCube::runBinDensityProg(int num_items) {
    clearDensity();
    gl::ScopedGlslProg prog(bin_density_prog_);
    bin_density_prog_->uniform("densityField", 0);
    bin_density_prog_->uniform("size", size_);
    bin_density_prog_->uniform("numItems", num_items);
    bin_density_prog_->uniform("res", resolution_);
    util::runProg(int(ceil(float(num_items) / float(WORK_GROUP_SIZE))));
}

void MarchingCube::runNormalizeDensityProg(const ivec3 thread) {
    gl::ScopedGlslProg prog(normalize_density_prog_);
    normalize_density_prog_->uniform("densityField", 0);
    normalize_density_prog_->uniform("res", resolution_);
    util::runProg(thread);
}

void MarchingCube::runMarchingCubeProg(float threshold, const ivec3 thread) {
    gl::ScopedBuffer scoped_grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(8);
    gl::ScopedBuffer scoped_flags_buffer(cube_edge_flags_buffer_);
    cube_edge_flags_buffer_->bindBase(9);
    gl::ScopedBuffer scoped_table_buffer(triangle_connection_table_buffer_);
    triangle_connection_table_buffer_->bindBase(10);

    gl::ScopedGlslProg prog(marching_cube_prog_);
    gl::ScopedVao vao(attributes_);

    marching_cube_prog_->uniform("densityField", 0);
    marching_cube_prog_->uniform("size", size_);
    marching_cube_prog_->uniform("res", resolution_);
    marching_cube_prog_->uniform("border", 1);
    marching_cube_prog_->uniform("threshold", threshold);

    util::runProg(thread);
}

void MarchingCube::update(int num_items, float threshold) {
    const ivec3 thread = ivec3(ceil(resolution_ / MARCHING_CUBE_GROUP_SIZE));

    density_field_->bind(0);

    runClearProg();
    runBinDensityProg(num_items);
    runNormalizeDensityProg(thread);
    runMarchingCubeProg(threshold, thread);

    density_field_->unbind(0);
}

void MarchingCube::draw() {
    // gl::setColor(ofColor::white);
    gl::ScopedDepthTest depthTest(true);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(8);

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedVao vao(attributes_);

    gl::context()->setDefaultShaderVars();
    gl::drawElements(GL_TRIANGLES, count_, GL_UNSIGNED_INT, nullptr);
}

MarchingCubeRef MarchingCube::create() { return std::make_shared<MarchingCube>(); }
