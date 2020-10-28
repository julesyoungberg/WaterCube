#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name)
    : BaseObject(name), size_(1.0f), num_particles_(1000), grid_res_(5), position_(0), gravity_(0, -9.8f, 0),
      particle_mass_(0.02f), kernel_radius_(0.1828f), viscosity_coefficient_(0.035f), stiffness_(250.0f),
      rest_density_(998.27f), rest_pressure_(0) {
    container_ = Container::create("fluidContainer", size_);
}

Fluid::~Fluid() {}

FluidRef Fluid::numParticles(int n) {
    num_particles_ = n;
    return std::make_shared<Fluid>(*this);
}

/**
 * Generates a vector of particles used as the simulations initial state
 */
void Fluid::generateInitialParticles() {
    OutputDebugStringA("creating particles\n");
    int n = num_particles_;
    initial_particles_.assign(n, Particle());

    const float azimuth = 256.0f * static_cast<float>(M_PI) / initial_particles_.size();
    const float inclination = static_cast<float>(M_PI) / initial_particles_.size();
    vec3 center = vec3(getWindowCenter() + vec2(0, 40.0f), 0.0f);

    for (int i = 0; i < n; i++) {
        // assign starting values to particles.
        float x = size_ * (math<float>::sin(inclination * i) * math<float>::cos(azimuth * i) * 0.5f + 0.5f);
        float y = size_ * (math<float>::cos(inclination * i) * 0.5f + 0.5f);
        float z = size_ * (math<float>::sin(inclination * i) * math<float>::sin(azimuth * i) * 0.5f + 0.5f);

        auto& p = initial_particles_.at(i);
        p.position = center + vec3(x, y, z);
        p.velocity = Rand::randVec3() * 10.0f; // random initial velocity
    }
}

/**
 * Prepares shared memory buffers
 * TODO: add distance function, wall weight function buffers
 */
void Fluid::prepareBuffers() {
    OutputDebugStringA("preparing fluid buffers\n");

    int n = num_particles_;
    std::vector<vec3> bin_velocities(num_bins_, vec3(0));

    // Create particle buffers on GPU and copy data into the first buffer.
    particle_buffer_1_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);
    particle_buffer_2_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);
    bin_velocity_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(vec3), bin_velocities.data(), GL_STATIC_DRAW);

    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });

    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
    attributes_ = gl::Vao::create();

    gl::ScopedBuffer scoped_bin_velocity(bin_velocity_buffer_);
    bin_velocity_buffer_->bindBase(8);
}

/**
 * Compile bin velocity compute shader
 */
void Fluid::compileBinVelocityProg() {
    OutputDebugStringA("\tcompiling fluid bin_velocity shader\n");
    bin_velocity_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/binVelocity.comp")));
}

/**
 * Compile density compute shader
 */
void Fluid::compileDensityProg() {
    OutputDebugStringA("\tcompiling fluid density shader\n");
    density_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/density.comp")));
}

/**
 * Compile update compute shader
 */
void Fluid::compileUpdateProg() {
    OutputDebugStringA("\tcompiling fluid update shader\n");
    update_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/update.comp")));
}

/**
 * Compil render shader
 */
void Fluid::compileRenderProg() {
    OutputDebugStringA("\tcompiling fluid render shader\n");
    render_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                            .vertex(loadAsset("fluid/particle.vert"))
                                            .fragment(loadAsset("fluid/particleGeometry.frag"))
                                            .attribLocation("particleId", 0));
}

/**
 * Compiles and prepares shader programs
 * TODO: handle error
 */
void Fluid::compileShaders() {
    OutputDebugStringA("compiling fluid shaders\n");

    gl::ScopedVao vao(attributes_);
    gl::ScopedBuffer scopedIds(ids_vbo_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

    compileBinVelocityProg();
    compileDensityProg();
    compileUpdateProg();
    compileRenderProg();
}

/**
 * setup simulation - initialize buffers and shaders
 */
FluidRef Fluid::setup() {
    OutputDebugStringA("initializing fluid\n");
    num_work_groups_ = num_particles_ / WORK_GROUP_SIZE;
    num_bins_ = int(pow(grid_res_, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_radius_ = bin_size_;

    viscosity_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    pressure_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    kernel_weight_ = static_cast<float>(315.0f / (64.0f * M_PI * std::pow(kernel_radius_, 9)));

    generateInitialParticles();

    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_work_groups_);

    OutputDebugStringA("creating buffers\n");
    prepareBuffers();

    OutputDebugStringA("initializing sorter\n");
    sort_ = Sort::create()->numItems(num_particles_)->numBins(num_bins_)->gridRes(grid_res_)->binSize(bin_size_);
    sort_->prepareBuffers();

    sort_->compileShaders();
    compileShaders();

    OutputDebugStringA("fluid created\n");
    return std::make_shared<Fluid>(*this);
}

/**
 * Generic run shader on particles - one for each
 */
void Fluid::runProg(int work_groups) {
    gl::dispatchCompute(work_groups, 1, 1);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::runProg() { runProg(num_work_groups_); }

/**
 * Run bin velocity compute shader
 */
void Fluid::runBinVelocityProg() {
    gl::ScopedGlslProg prog(bin_velocity_prog_);
    runProg(num_bins_);
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg() {
    gl::ScopedGlslProg prog(density_prog_);
    density_prog_->uniform("binSize", bin_size_);
    density_prog_->uniform("gridRes", grid_res_);
    density_prog_->uniform("mass", particle_mass_);
    density_prog_->uniform("kernelRadius", kernel_radius_);
    density_prog_->uniform("kernelWeight", kernel_weight_);
    density_prog_->uniform("stiffness", stiffness_);
    density_prog_->uniform("restDensity", rest_density_);
    density_prog_->uniform("restPressure", rest_pressure_);
    runProg();
}

/**
 * Run update compute shader
 */
void Fluid::runUpdateProg(float time_step) {
    gl::ScopedGlslProg prog(update_prog_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("dt", time_step);
    update_prog_->uniform("gravity", gravity_);
    update_prog_->uniform("mass", particle_mass_);
    update_prog_->uniform("kernelRadius", kernel_radius_);
    update_prog_->uniform("viscosityCoefficient", viscosity_coefficient_);
    update_prog_->uniform("viscosityWeight", viscosity_weight_);
    update_prog_->uniform("pressureWeight", pressure_weight_);
    runProg();
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    odd_frame_ = !odd_frame_;
    gl::ScopedBuffer scoped_particle_buffer1(particle_buffer_1_);
    gl::ScopedBuffer scoped_particle_buffer2(particle_buffer_2_);
    particle_buffer_1_->bindBase(odd_frame_ ? 0 : 1);
    particle_buffer_2_->bindBase(odd_frame_ ? 1 : 0);
    gl::ScopedBuffer scoped_bin_velocity(bin_velocity_buffer_);

    sort_->run();

    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());

    runBinVelocityProg();
    runDensityProg();
    runUpdateProg(time);
}

/**
 * Draw simulation logic
 */
void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::pushMatrices();
    float half = size_ / 2.0f;
    vec3 center = vec3(getWindowCenter(), 0) + vec3(-half, -half, -half);
    gl::translate(center.x, center.y, center.z);

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedVao vao(attributes_);

    gl::ScopedBuffer scoped_bucket_ssbo(sort_->bucketBuffer());
    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    gl::ScopedBuffer scoped_bin_velocity_ssbo(bin_velocity_buffer_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

void Fluid::reset() {}

/**
 * Create a new fluid simulation with a smart pointer
 */
FluidRef Fluid::create(const std::string& name) { return std::make_shared<Fluid>(name); }
