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
    void run(GLuint in_particles, GLuint out_particles);
    void renderGrid(float size);

    GLuint getCountBuffer() { return count_buffer_; }
    GLuint getOffsetBuffer() { return offset_buffer_; }

    static SortRef create() { return std::make_shared<Sort>(); }

protected:
    void clearCount();
    void clearCountBuffer();
    void clearOffsetBuffer();
    void printGrids();
    void prepareGridParticles();

    void runProg() { util::runProg(int(ceil(float(num_items_) / float(WORK_GROUP_SIZE)))); }
    void runCountProg(GLuint particle_buffer);
    void runLinearScanProg();
    void runScanProg();
    void runReorderProg(GLuint in_particles, GLuint out_particles);

    SortRef thisRef() { return std::make_shared<Sort>(*this); }

    int num_items_, num_bins_, grid_res_;
    float bin_size_;
    bool use_linear_scan_;

    std::vector<ivec4> grid_particles_;

    gl::GlslProgRef count_prog_, linear_scan_prog_, scan_prog_, reorder_prog_, render_grid_prog_;
    gl::SsboRef position_buffer_, global_count_buffer_, grid_buffer_;
    gl::Texture1dRef id_map_;
    gl::VboRef grid_ids_vbo_;
    gl::VaoRef grid_attributes_;

    GLuint count_buffer_, offset_buffer_;
};

} // namespace core
