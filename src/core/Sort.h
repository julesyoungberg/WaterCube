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
    void run(gl::SsboRef in_particles, gl::SsboRef out_particles);
    std::vector<Particle> runCpu(std::vector<Particle> in_particles);
    void runCpu(gl::SsboRef in_particles_buffer, gl::SsboRef out_particles_buffer);
    void renderGrid(float size);

    gl::Texture3dRef getCountGrid() { return count_grid_; }
    gl::Texture3dRef getOffsetGrid() { return offset_grid_; }

    static SortRef create();

protected:
    int num_items_, num_bins_, grid_res_;
    float bin_size_;
    bool use_linear_scan_;

    std::vector<ivec3> grid_particles_;

    gl::GlslProgRef count_prog_, linear_scan_prog_, scan_prog_, reorder_prog_, render_grid_prog_, sort_prog_;
    gl::SsboRef position_buffer_, count_buffer_, grid_buffer_;
    gl::Texture1dRef id_map_;
    gl::Texture3dRef count_grid_, offset_grid_;
    gl::VboRef grid_ids_vbo_;
    gl::VaoRef grid_attributes_;

private:
    void clearCountGrid();
    void clearOffsetGrid();
    void clearCount();
    void printGrids();

    // GPU STUFF
    void runProg();
    void runCountProg(gl::SsboRef particles);
    void runLinearScanProg();
    void runScanProg();
    void runReorderProg(gl::SsboRef in_particles, gl::SsboRef out_particles);
    void runSortProg(gl::SsboRef particles);

    std::vector<Particle> getParticles(gl::SsboRef particle_buffer);

    // CPU STUFF
    std::vector<uint32_t> count(std::vector<Particle> particles);
    std::vector<uint32_t> countOffsets(std::vector<uint32_t> counts);
    std::vector<Particle> reorder(std::vector<Particle> in_particles, std::vector<uint32_t> offsets);
    void saveCountsToTexture(std::vector<uint32_t> counts);
    void saveOffsetsToTexture(std::vector<uint32_t> offsets);

    std::vector<uint32_t> cpuSort(std::vector<Particle> particles, std::vector<uint32_t> offsets);

    SortRef thisRef() { return std::make_shared<Sort>(*this); }
};

} // namespace core
