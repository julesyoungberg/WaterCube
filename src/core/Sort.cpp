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
    OutputDebugStringA("preparing sort buffers\n");

    std::vector<int> buckets(num_items_, 0);
    std::vector<int> counts(num_bins_, 0);
    std::vector<int> offsets(num_bins_, 0);
    std::vector<int> sorted(num_items_, 0);

    bucket_buffer_ = gl::Ssbo::create(num_items_ * sizeof(int), buckets.data(), GL_STATIC_DRAW);
    count_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(int), counts.data(), GL_STATIC_DRAW);
    offset_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(int), offsets.data(), GL_STATIC_DRAW);
    sorted_buffer_ = gl::Ssbo::create(num_items_ * sizeof(int), sorted.data(), GL_STATIC_DRAW);
}

/**
 * Compile bucket compute shader
 */
void Sort::compileBucketProg() {
    OutputDebugStringA("\tcompiling sorter bucket shader\n");

    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    bucket_buffer_->bindBase(1);

    bucket_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/bucket.comp")));
}

/**
 * Compile count compute shader
 */
void Sort::compileCountProg() {
    OutputDebugStringA("\tcompiling sorter count shader\n");

    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    bucket_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    count_buffer_->bindBase(1);

    count_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/count.comp")));
}

/**
 * Compile scan compute shader
 */
void Sort::compileScanProg() {
    OutputDebugStringA("\tcompiling sorter scan shader\n");

    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    count_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_offset_ssbo(offset_buffer_);
    offset_buffer_->bindBase(1);

    scan_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/scan.comp")));
}

/**
 * Compile reorder compute shader
 */
void Sort::compileReorderProg() {
    OutputDebugStringA("\tcompiling sorter reorder shader\n");

    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    bucket_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_offset_ssbo(offset_buffer_);
    offset_buffer_->bindBase(1);

    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    count_buffer_->bindBase(2);

    gl::ScopedBuffer scoped_sorted_ssbo(sorted_buffer_);
    sorted_buffer_->bindBase(3);

    reorder_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("sort/reorder.comp")));
}

/**
 * Compiles and prepares shader programs
 * TODO: handle error
 */
void Sort::compileShaders() {
    OutputDebugStringA("compiling sort shaders\n");
    compileBucketProg();
    compileCountProg();
    compileScanProg();
    compileReorderProg();
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
    bucket_prog_->uniform("gridRes", grid_res_);
    bucket_prog_->uniform("binSize", bin_size_);
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    runProg();
}

/**
 * Run bucket count compute shader
 * TODO reset count buffer
 */
void Sort::runCountProg() {
    gl::ScopedGlslProg prog(count_prog_);
    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    runProg();
}

/**
 * Run bucket scan compute shader
 * TODO reset offset buffer
 * TODO parallelize
 */
void Sort::runScanProg() {
    gl::ScopedGlslProg prog(scan_prog_);
    scan_prog_->uniform("numBins", num_bins_);
    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    gl::ScopedBuffer scoped_offset_ssbo(offset_buffer_);
    runProg(1);
}

/**
 * Run reorder compute shader
 * TODO reset count buffer
 */
void Sort::runReorderProg() {
    gl::ScopedGlslProg prog(reorder_prog_);
    gl::ScopedBuffer scoped_bucket_ssbo(bucket_buffer_);
    gl::ScopedBuffer scoped_offset_ssbo(offset_buffer_);
    gl::ScopedBuffer scoped_count_ssbo(count_buffer_);
    gl::ScopedBuffer scoped_sorted_ssbo(sorted_buffer_);
    runProg();
}

void Sort::run() {
    runBucketProg();
    runCountProg();
    runScanProg();
    runReorderProg();
}

SortRef Sort::create() { return std::make_shared<Sort>(); }
