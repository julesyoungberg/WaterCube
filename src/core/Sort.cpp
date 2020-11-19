#include "./Sort.h"

using namespace core;

Sort::Sort() {
    num_items_ = 0;
    num_bins_ = 1;
    use_linear_scan_ = true;
}

Sort::~Sort() {
    glDeleteBuffers(1, &count_buffer_);
    glDeleteBuffers(1, &offset_buffer_);
}

SortRef Sort::numItems(int n) {
    num_items_ = n;
    return thisRef();
}

SortRef Sort::gridRes(int r) {
    grid_res_ = r;
    num_bins_ = int(pow(grid_res_, 3));
    return thisRef();
}

SortRef Sort::binSize(float s) {
    bin_size_ = s;
    return thisRef();
}

SortRef Sort::positionBuffer(gl::SsboRef buffer) {
    position_buffer_ = buffer;
    return thisRef();
}

/**
 * prepare debugging grid particles
 */
void Sort::prepareGridParticles() {
    util::log("\tcreating grid particles");
    grid_particles_.resize(int(pow(grid_res_, 3)));
    for (int z = 0; z < grid_res_; z++) {
        for (int y = 0; y < grid_res_; y++) {
            for (int x = 0; x < grid_res_; x++) {
                grid_particles_.push_back(ivec4(x, y, z, 0));
            }
        }
    }
    grid_buffer_ = gl::Ssbo::create(grid_particles_.size() * sizeof(ivec4), grid_particles_.data(),
                                    GL_STATIC_DRAW);

    util::log("\tcreating grid particles ids vbo");
    std::vector<GLuint> ids(grid_particles_.size());
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    grid_ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating grid particles attributes vao");
    grid_attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedDids(grid_ids_vbo_);
    gl::ScopedVao grid_vao(grid_attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
}

/**
 * Prepares shared memory buffers
 */
void Sort::prepareBuffers() {
    util::log("preparing sort buffers");

    util::log("\tcreating count buffer");
    global_count_buffer_ = gl::Ssbo::create(sizeof(int), nullptr, GL_DYNAMIC_STORAGE_BIT);
    gl::ScopedBuffer scoped_count_buffer(global_count_buffer_);
    global_count_buffer_->bindBase(8);

    util::log("\tcreating count and offset grids");
    std::vector<uint32_t> zeros(num_items_, 0);
    glCreateBuffers(1, &count_buffer_);
    glNamedBufferStorage(count_buffer_, num_items_ * sizeof(uint32_t), zeros.data(), 0);
    glCreateBuffers(1, &offset_buffer_);
    glNamedBufferStorage(offset_buffer_, num_items_ * sizeof(uint32_t), zeros.data(), 0);

    prepareGridParticles();

    util::log("\tcreating id map");
    std::vector<uint32_t> sids(num_items_);
    uint32_t curr_id = 0;
    std::generate(sids.begin(), sids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    auto id_format = gl::Texture1d::Format().internalFormat(GL_R32UI);
    id_map_ = gl::Texture1d::create(sids.data(), GL_R32F, num_items_, id_format);
}

/**
 * Compiles and prepares shader programs
 */
void Sort::compileShaders() {
    util::log("compiling sort shaders");

    util::log("\tcompiling sorter count shader");
    count_prog_ = util::compileComputeShader("sort/count.comp");

    util::log("\tcompiling sorter linear scan shader");
    linear_scan_prog_ = util::compileComputeShader("sort/linearScan.comp");

    util::log("\tcompiling sorter scan shader");
    scan_prog_ = util::compileComputeShader("sort/scan.comp");

    util::log("\tcompiling sorter reorder shader");
    reorder_prog_ = util::compileComputeShader("sort/reorder.comp");

    util::log("\tcompiling render grid shader");
    render_grid_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                                 .vertex(loadAsset("sort/grid.vert"))
                                                 .fragment(loadAsset("sort/grid.frag"))
                                                 .attribLocation("gridID", 0));
}

/**
 * clear global count buffer
 */
void Sort::clearCount() {
    const std::uint32_t clear_value = 0;
    gl::ScopedBuffer count_buffer(global_count_buffer_);
    glClearBufferData(global_count_buffer_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                      &clear_value);
    gl::memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

/**
 * clear counter buffer
 */
void Sort::clearCountBuffer() {
    std::vector<uint32_t> initial(num_items_, 0);
    gl::ScopedBuffer buffer(GL_SHADER_STORAGE_BUFFER, count_buffer_);
    glClearNamedBufferData(count_buffer_, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                           initial.data());
    gl::memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

/**
 * clear offset buffer
 */
void Sort::clearOffsetBuffer() {
    std::vector<uint32_t> initial(num_items_, 0);
    gl::ScopedBuffer buffer(GL_SHADER_STORAGE_BUFFER, count_buffer_);
    glClearNamedBufferData(offset_buffer_, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT,
                           initial.data());
    gl::memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

/**
 * Run bucket count compute shader
 */
void Sort::runCountProg(GLuint particle_buffer) {
    gl::ScopedGlslProg prog(count_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, count_buffer_);

    count_prog_->uniform("binSize", bin_size_);
    count_prog_->uniform("numItems", num_items_);
    count_prog_->uniform("gridRes", grid_res_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run a single shader to scan the counts and compute prefix sums (offsets)
 * PERFORMANCE BOTTLENECK
 */
void Sort::runLinearScanProg() {
    gl::ScopedGlslProg prog(linear_scan_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, count_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, offset_buffer_);

    linear_scan_prog_->uniform("gridRes", grid_res_);

    util::runProg(1);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run bucket scan compute shader
 */
void Sort::runScanProg() {
    gl::ScopedGlslProg prog(scan_prog_);

    gl::ScopedBuffer scoped_count_buffer(global_count_buffer_);
    global_count_buffer_->bindBase(0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, count_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, offset_buffer_);

    scan_prog_->uniform("gridRes", grid_res_);

    util::runProg(ivec3(int(ceil(num_bins_ / 4.0f))));
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run reorder compute shader
 */
void Sort::runReorderProg(GLuint in_particles, GLuint out_particles) {
    gl::ScopedGlslProg prog(reorder_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, in_particles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, out_particles);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, count_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, offset_buffer_);

    reorder_prog_->uniform("binSize", bin_size_);
    reorder_prog_->uniform("numItems", num_items_);
    reorder_prog_->uniform("gridRes", grid_res_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * print current values in count and offset buffers
 */
void Sort::printGrids() {
    std::vector<uint32_t> counts = util::getUints(count_buffer_, num_bins_);
    std::vector<uint32_t> offsets = util::getUints(offset_buffer_, num_bins_);

    std::string c = "counts: ";
    std::string o = "offsets: ";
    for (int i = 0; i < 100; i++) {
        c += std::to_string(counts[i]) + ", ";
        o += std::to_string(offsets[i]) + ", ";
    }
    util::log("%s", c.c_str());
    util::log("%s", o.c_str());
}

/**
 * main logic - sort in_particles and store result in out_particles
 */
void Sort::run(GLuint in_particles, GLuint out_particles) {
    clearCountBuffer();
    runCountProg(in_particles);

    clearOffsetBuffer();
    if (use_linear_scan_) {
        runLinearScanProg();
    } else {
        runScanProg();
    }

    clearCountBuffer();
    runReorderProg(in_particles, out_particles);
}

/**
 * render debugging grid
 */
void Sort::renderGrid(float size) {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_grid_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, count_buffer_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, offset_buffer_);

    render_grid_prog_->uniform("binSize", bin_size_);
    render_grid_prog_->uniform("gridRes", grid_res_);
    render_grid_prog_->uniform("size", size);
    render_grid_prog_->uniform("numItems", num_items_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, int(grid_particles_.size()));
}
