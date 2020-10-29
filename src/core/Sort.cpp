#include "./Sort.h"

using namespace core;

Sort::Sort() : num_items_(0), num_bins_(1) {}

Sort::~Sort() {}

SortRef Sort::numItems(int n) {
    num_items_ = n;
    num_work_groups_ = n / WORK_GROUP_SIZE;
    return thisRef();
}

SortRef Sort::numBins(int n) {
    num_bins_ = n;
    return thisRef();
}

SortRef Sort::gridRes(int r) {
    grid_res_ = r;
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

    std::vector<int> buckets(num_items_, 0);
    std::vector<int> counts(num_bins_, 0);
    std::vector<int> offsets(num_bins_, 0);

    bucket_buffer_ = gl::Ssbo::create(num_items_ * sizeof(int), buckets.data(), GL_STATIC_DRAW);
    count_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(int), counts.data(), GL_STATIC_DRAW);
    offset_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(int), offsets.data(), GL_STATIC_DRAW);

    grid_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_);
}

/**
 * Compile bucket compute shader
 */
void Sort::compileBucketProg() {
    util::log("\tcompiling sorter bucket shader");
    bucket_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/bucket.comp")));
}

/**
 * Compile count compute shader
 */
void Sort::compileCountProg() {
    util::log("\tcompiling sorter count shader");
    count_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/count.comp")));
}

/**
 * Compile scan compute shader
 */
void Sort::compileScanProg() {
    util::log("\tcompiling sorter scan shader");
    scan_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/scan.comp")));
}

/**
 * Compile reorder compute shader
 */
void Sort::compileReorderProg() {
    util::log("\tcompiling sorter reorder shader");
    reorder_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/reorder.comp")));
}

/**
 * Compiles and prepares shader programs
 * TODO: handle error
 */
void Sort::compileShaders() {
    util::log("compiling sort shaders");

    gl::ScopedBuffer scoped_bucket(bucket_buffer_);
    bucket_buffer_->bindBase(16);
    gl::ScopedBuffer scoped_offset(offset_buffer_);
    offset_buffer_->bindBase(17);
    gl::ScopedBuffer scoped_count(count_buffer_);
    count_buffer_->bindBase(18);

    compileBucketProg();
    compileCountProg();
    compileScanProg();
    compileReorderProg();
}

/**
 * Clears the count bin count buffer
 */
void Sort::clearCount() {
    const std::uint32_t clear_value = 0;
    glClearBufferData(count_buffer_->getTarget(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_value);
}

/**
 * Generic run shader on particles - one for each
 */
void Sort::runProg(int work_groups) {
    gl::dispatchCompute(work_groups, 1, 1);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Sort::runProg() { runProg(num_work_groups_); }

/**
 * Run bucket compute shader
 */
void Sort::runBucketProg() {
    gl::ScopedGlslProg prog(bucket_prog_);
    // grid_->bind(0);
    // bucket_prog_->uniform("grid", 0);
    bucket_prog_->uniform("gridRes", grid_res_);
    bucket_prog_->uniform("binSize", bin_size_);
    runProg();
}

/**
 * Run bucket count compute shader
 * TODO reset count buffer
 */
void Sort::runCountProg() {
    clearCount();
    gl::ScopedGlslProg prog(count_prog_);
    runProg();
}

/**
 * Run bucket scan compute shader
 * TODO reset offset buffer
 * TODO parallelize
 */
void Sort::runScanProg() {
    clearCount();
    gl::ScopedGlslProg prog(scan_prog_);
    scan_prog_->uniform("numBins", num_bins_);
    runProg(1);
}

/**
 * Run reorder compute shader
 * TODO reset count buffer
 */
void Sort::runReorderProg() {
    gl::ScopedGlslProg prog(reorder_prog_);
    runProg();
}

void Sort::run() {
    gl::ScopedBuffer scoped_bucket(bucket_buffer_);
    gl::ScopedBuffer scoped_offset(offset_buffer_);
    gl::ScopedBuffer scoped_count(count_buffer_);

    runBucketProg();
    runCountProg();
    runScanProg();
    runReorderProg();
}

SortRef Sort::create() { return std::make_shared<Sort>(); }
