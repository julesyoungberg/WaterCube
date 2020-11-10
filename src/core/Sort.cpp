#include "./Sort.h"

using namespace core;

Sort::Sort() : num_items_(0), num_bins_(1) {}

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

    count_buffer_ = gl::Ssbo::create(sizeof(int), nullptr, GL_DYNAMIC_STORAGE_BIT);
    gl::ScopedBuffer scoped_count_buffer(count_buffer_);
    count_buffer_->bindBase(8);

    auto format = gl::Texture3d::Format().internalFormat(GL_R32UI);
    count_grid_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, format);
    offset_grid_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, format);
}

/**
 * Compiles and prepares shader programs
 */
void Sort::compileShaders() {
    util::log("compiling sort shaders");

    util::log("\tcompiling sorter count shader");
    count_prog_ = util::compileComputeShader("sort/count.comp");

    util::log("\tcompiling sorter scan shader");
    scan_prog_ = util::compileComputeShader("sort/scan.comp");

    util::log("\tcompiling sorter reorder shader");
    reorder_prog_ = util::compileComputeShader("sort/reorder.comp");
}

void Sort::clearCountGrid() {
    const std::uint32_t clear_value = 0;
    glClearTexImage(count_grid_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
}

void Sort::clearCount() {
    const std::uint32_t clear_value = 0;
    glClearBufferData(count_buffer_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
}

void Sort::runProg() { util::runProg(int(ceil(float(num_items_) / float(WORK_GROUP_SIZE)))); }

/**
 * Run bucket count compute shader
 */
void Sort::runCountProg() {
    clearCountGrid();
    gl::ScopedGlslProg prog(count_prog_);
    count_prog_->uniform("countGrid", 0);
    count_prog_->uniform("binSize", bin_size_);
    count_prog_->uniform("numItems", num_items_);
    runProg();
}

/**
 * Run bucket scan compute shader
 */
void Sort::runScanProg() {
    clearCount();
    gl::ScopedGlslProg prog(scan_prog_);
    scan_prog_->uniform("countGrid", 0);
    scan_prog_->uniform("offsetGrid", 1);
    scan_prog_->uniform("gridRes", grid_res_);
    gl::ScopedBuffer scoped_count_buffer(count_buffer_);
    util::runProg(ivec3(int(ceil(num_bins_ / 4.0f))));
}

/**
 * Run reorder compute shader
 */
void Sort::runReorderProg() {
    clearCountGrid();
    gl::ScopedGlslProg prog(reorder_prog_);
    reorder_prog_->uniform("binSize", bin_size_);
    reorder_prog_->uniform("numItems", num_items_);
    runProg();
}

void Sort::run() {
    count_grid_->bind(0);
    offset_grid_->bind(1);

    runCountProg();
    runScanProg();
    runReorderProg();

    count_grid_->unbind(0);
    offset_grid_->unbind(1);
}

SortRef Sort::create() { return std::make_shared<Sort>(); }
