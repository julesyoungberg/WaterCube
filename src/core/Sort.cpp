#include "./Sort.h"

using namespace core;

Sort::Sort() {
    num_items_ = 0;
    num_bins_ = 1;
    use_linear_scan_ = true;
}

Sort::~Sort() {}

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
 * Prepares shared memory buffers
 */
void Sort::prepareBuffers() {
    util::log("preparing sort buffers");

    util::log("\tcreating count buffer");
    count_buffer_ = gl::Ssbo::create(sizeof(int), nullptr, GL_DYNAMIC_STORAGE_BIT);
    gl::ScopedBuffer scoped_count_buffer(count_buffer_);
    count_buffer_->bindBase(8);

    util::log("\tcreating count and offset grids");
    auto format = gl::Texture3d::Format().internalFormat(GL_R32UI);
    count_grid_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, format);
    offset_grid_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, format);

    util::log("\tcreating grid particles");
    grid_particles_.resize(int(pow(grid_res_, 3)));
    for (int z = 0; z < grid_res_; z++) {
        for (int y = 0; y < grid_res_; y++) {
            for (int x = 0; x < grid_res_; x++) {
                grid_particles_.push_back(ivec3(x, y, z));
            }
        }
    }
    grid_buffer_ = gl::Ssbo::create(grid_particles_.size() * sizeof(ivec3), grid_particles_.data(), GL_STATIC_DRAW);

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

void Sort::clearCountGrid() {
    const std::vector<GLubyte> empty_data(num_bins_, 0);
    gl::ScopedTextureBind tbScope(count_grid_->getTarget(), count_grid_->getId());
    glTexSubImage3D(count_grid_->getTarget(), 0, 0, 0, 0, grid_res_, grid_res_, grid_res_, GL_RED, GL_UNSIGNED_INT,
                    empty_data.data());
    // glClearTexImage(count_grid_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, clear_value.data());
    // glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void Sort::clearOffsetGrid() {
    const std::uint32_t clear_value = 0;
    gl::ScopedTextureBind tbScope(offset_grid_->getTarget(), offset_grid_->getId());
    glClearTexImage(offset_grid_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void Sort::clearCount() {
    const std::uint32_t clear_value = 0;
    gl::ScopedBuffer count_buffer(count_buffer_);
    glClearBufferData(count_buffer_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

void Sort::runProg() { util::runProg(int(ceil(float(num_items_) / float(WORK_GROUP_SIZE)))); }

/**
 * Run bucket count compute shader
 */
void Sort::runCountProg(gl::SsboRef particles) {
    gl::ScopedGlslProg prog(count_prog_);

    gl::ScopedBuffer scoped_particles(particles);
    particles->bindBase(0);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(count_prog_->getHandle(), "countGrid"), 1);
    glBindImageTexture(1, count_grid_->getId(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    count_prog_->uniform("binSize", bin_size_);
    count_prog_->uniform("numItems", num_items_);
    runProg();
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    particles->unbindBase();
}

void Sort::runLinearScanProg() {
    gl::ScopedGlslProg prog(linear_scan_prog_);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(linear_scan_prog_->getHandle(), "offsetGrid"), 0);
    glBindImageTexture(0, offset_grid_->getId(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    count_grid_->bind(1);

    linear_scan_prog_->uniform("countGrid", 1);
    linear_scan_prog_->uniform("gridRes", grid_res_);

    util::runProg(1);
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    count_grid_->unbind(0);
}

/**
 * Run bucket scan compute shader
 */
void Sort::runScanProg() {
    gl::ScopedGlslProg prog(scan_prog_);

    gl::ScopedBuffer scoped_count_buffer(count_buffer_);
    count_buffer_->bindBase(0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glUniform1i(glGetUniformLocation(scan_prog_->getHandle(), "offsetGrid"), 1);
    glBindImageTexture(1, offset_grid_->getId(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);

    count_grid_->bind(2);

    scan_prog_->uniform("countGrid", 2);
    scan_prog_->uniform("gridRes", grid_res_);

    util::runProg(ivec3(ceil(num_bins_ / 4.0f)));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    count_grid_->unbind(2);
    count_buffer_->unbindBase();
}

/**
 * Run reorder compute shader
 */
void Sort::runReorderProg(gl::SsboRef in_particles, gl::SsboRef out_particles) {
    gl::ScopedGlslProg prog(reorder_prog_);

    gl::ScopedBuffer scoped_in_particles(in_particles);
    in_particles->bindBase(0);

    gl::ScopedBuffer scoped_out_particles(out_particles);
    out_particles->bindBase(1);

    count_grid_->bind(2);
    offset_grid_->bind(3);
    reorder_prog_->uniform("countGrid", 2);
    reorder_prog_->uniform("offsetGrid", 3);

    reorder_prog_->uniform("binSize", bin_size_);
    reorder_prog_->uniform("numItems", num_items_);

    runProg();
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    count_grid_->unbind(2);
    offset_grid_->unbind(3);

    in_particles->unbindBase();
    out_particles->unbindBase();
}

void Sort::run(gl::SsboRef in_particles, gl::SsboRef out_particles) {
    clearCountGrid();

    runCountProg(in_particles);
    clearCountGrid();
    // runCpuCount(in_particles);

    gl::ScopedTextureBind tbScope(count_grid_->getTarget(), count_grid_->getId());
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    std::vector<uint32_t> counts(num_bins_);
    glGetTexImage(count_grid_->getTarget(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, counts.data());

    std::string c = "counts: ";
    // std::string o = "offsets: ";
    for (int i = 0; i < 100; i++) {
        c += std::to_string(counts[i]) + ", ";
        // o += std::to_string(offsets[i]) + ", ";
    }
    util::log("%s", c.c_str());
    // util::log("%s", o.c_str());

    clearOffsetGrid();
    // if (use_linear_scan_) {
    //     runLinearScanProg();
    // } else {
    //     runScanProg();
    // }

    // clearCountGrid();
    // runReorderProg(in_particles, out_particles);
}

std::vector<int> Sort::count(std::vector<Particle> particles) {
    std::vector<int> counts(int(pow(grid_res_, 3)), 0);

    for (auto const& p : particles) {
        // util::log("particle: <%f, %f, %f>", p.position.x, p.position.y, p.position.z);
        const ivec3 coord = ivec3(p.position / bin_size_);
        const int index = coord.z * grid_res_ * grid_res_ + coord.y * grid_res_ + coord.x;
        // util::log("binCoord: <%d, %d, %d> - %d", coord.x, coord.y, coord.z, index);
        counts[index] += 1;
    }

    return counts;
}

std::vector<int> Sort::countOffsets(std::vector<int> counts) {
    std::vector<int> offsets(int(counts.size()), 0);
    int numNonZero = 0;

    for (int i = 1; i < int(counts.size()); i++) {
        offsets[i] = offsets[i - 1] + counts[i - 1];
        if (counts[i - 1] > 0) {
            numNonZero++;
        }
    }

    util::log("num non zero: %d of %d", numNonZero, num_bins_);

    return offsets;
}

std::vector<Particle> Sort::reorder(std::vector<Particle> in_particles, std::vector<int> offsets) {
    std::vector<int> counts(int(pow(grid_res_, 3)), 0);
    std::vector<Particle> reordered(int(in_particles.size()));

    for (auto const& p : in_particles) {
        const ivec3 coord = ivec3(p.position / bin_size_);
        const int index = coord.z * grid_res_ * grid_res_ + coord.y * grid_res_ + coord.x;
        const int binOffset = offsets[index];
        const int binIndex = counts[index]++;
        reordered[binOffset + binIndex] = p;
    }

    return reordered;
}

void Sort::saveCountsToTexture(std::vector<int> counts) {
    auto format = gl::Texture3d::Format().internalFormat(GL_R32F);
    count_grid_ = gl::Texture3d::create(counts.data(), GL_R32F, grid_res_, grid_res_, grid_res_, format);
}

void Sort::saveOffsetsToTexture(std::vector<int> offsets) {
    auto format = gl::Texture3d::Format().internalFormat(GL_R32F);
    offset_grid_ = gl::Texture3d::create(offsets.data(), GL_R32F, grid_res_, grid_res_, grid_res_, format);
}

std::vector<Particle> Sort::runCpu(std::vector<Particle> in_particles) {
    auto counts = count(in_particles);
    saveCountsToTexture(counts);
    auto offsets = countOffsets(counts);
    // std::string c = "counts: ";
    // std::string o = "offsets: ";
    // for (int i = 0; i < 100; i++) {
    //     c += std::to_string(counts[i]) + ", ";
    //     o += std::to_string(offsets[i]) + ", ";
    // }
    // util::log("%s", c.c_str());
    // util::log("%s", o.c_str());
    saveOffsetsToTexture(offsets);
    auto reordered = reorder(in_particles, offsets);
    return reordered;
}

void Sort::runCpu(gl::SsboRef in_particles_buffer, gl::SsboRef out_particles_buffer) {
    auto in_particles = util::getParticles(in_particles_buffer, num_items_);
    auto reordered = runCpu(in_particles);
    glBufferData(out_particles_buffer->getTarget(), reordered.size() * sizeof(Particle), reordered.data(),
                 GL_DYNAMIC_DRAW);
}

void Sort::renderGrid(float size) {
    gl::pointSize(10);

    gl::ScopedGlslProg render(render_grid_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    count_grid_->bind(1);
    offset_grid_->bind(2);
    render_grid_prog_->uniform("countGrid", 1);
    render_grid_prog_->uniform("offsetGrid", 2);

    render_grid_prog_->uniform("binSize", bin_size_);
    render_grid_prog_->uniform("gridRes", grid_res_);
    render_grid_prog_->uniform("size", size);
    render_grid_prog_->uniform("numItems", num_items_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, grid_particles_.size());

    count_grid_->unbind(1);
    offset_grid_->unbind(2);
}

SortRef Sort::create() { return std::make_shared<Sort>(); }
