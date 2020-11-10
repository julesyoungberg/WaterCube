#pragma once

#include <Windows.h>
#include <memory>

#include "cinder/app/App.h"
#include "cinder/gl/Shader.h"
#include "cinder/gl/Ssbo.h"
#include "cinder/gl/gl.h"

#include "./util.h"

using namespace ci;
using namespace ci::app;

namespace core {

typedef std::shared_ptr<class Sort> SortRef;

class Sort {
public:
    Sort();
    ~Sort();

    SortRef numItems(int n);
    SortRef gridRes(int r);
    SortRef binSize(float s);
    SortRef positionBuffer(gl::SsboRef buffer);

    void prepareBuffers();
    void compileShaders();
    void run();

    gl::Texture3dRef getCountGrid() { return count_grid_; }
    gl::Texture3dRef getOffsetGrid() { return offset_grid_; }

    static SortRef create();

protected:
    int num_items_, num_bins_, grid_res_;
    float bin_size_;
    bool use_linear_scan_;

    gl::GlslProgRef count_prog_, linear_scan_prog_, scan_prog_, reorder_prog_;
    gl::SsboRef position_buffer_, count_buffer_;
    gl::Texture3dRef count_grid_, offset_grid_;

private:
    void clearCountGrid();
    void clearCount();

    void runProg();
    void runCountProg();
    void runLinearScanProg();
    void runScanProg();
    void runReorderProg();

    SortRef thisRef() { return std::make_shared<Sort>(*this); }
};

} // namespace core
