#pragma once

#include <Windows.h>
#include <memory>

#include "cinder/app/App.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"

#include "./util.h"

using namespace ci;
using namespace ci::app;

namespace core {

const int WORK_GROUP_SIZE = 128;
const int CHUNK_SIZE = WORK_GROUP_SIZE * 4;

typedef std::shared_ptr<class Sort> SortRef;

class Sort {
public:
    Sort();
    ~Sort();

    SortRef numItems(int n);
    SortRef numBins(int n);
    SortRef gridRes(int r);
    SortRef binSize(float s);
    SortRef positionBuffer(gl::SsboRef buffer);

    void prepareBuffers();
    void compileShaders();
    void run();

    gl::SsboRef bucketBuffer() { return bucket_buffer_; }
    gl::SsboRef countBuffer() { return count_buffer_; }
    gl::SsboRef offsetBuffer() { return offset_buffer_; }

    static SortRef create();

protected:
    int num_items_, num_bins_, num_work_groups_, grid_res_;
    float bin_size_;

    gl::GlslProgRef bucket_prog_, count_prog_, scan_prog_, reorder_prog_;
    gl::SsboRef position_buffer_, bucket_buffer_, count_buffer_, offset_buffer_;
    gl::Texture3dRef grid_;

private:
    void compileBucketProg();
    void compileCountProg();
    void compileScanProg();
    void compileReorderProg();

    void clearCount();

    void runProg(int work_groups);
    void runProg();
    void runBucketProg();
    void runCountProg();
    void runScanProg();
    void runReorderProg();

    SortRef thisRef() { return std::make_shared<Sort>(*this); }
};

} // namespace core
