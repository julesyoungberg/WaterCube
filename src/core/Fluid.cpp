#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name)
    : BaseObject(name), size_(100), num_particles_(1000), grid_res_(100), position_(0), gravity_(0, -9.8, 0),
      particle_mass_(1), kernel_(10), viscosity_coefficient_(1), viscosity_weight_(1), pressure_weight_(1),
      weight_constant_(1), k_(1), rest_density_(1), rest_pressure_(1) {
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
    vec3 center = vec3(getWindowCenter() + vec2(0.0f, 40.0f), 0.0f);

    for (int i = 0; i < n; i++) {
        // assign starting values to particles.
        float x = size_ * (math<float>::sin(inclination * i) * math<float>::cos(azimuth * i) * 0.5 + 0.5);
        float y = size_ * (math<float>::cos(inclination * i) * 0.5 + 0.5);
        float z = size_ * (math<float>::sin(inclination * i) * math<float>::sin(azimuth * i) * 0.5 + 0.5);

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
    std::vector<vec3> positions(n, vec3(0));
    std::vector<vec3> velocities(n, vec3(0));
    std::vector<float> densities(n, 1);
    std::vector<float> pressures(n, 1);
    std::vector<vec3> bin_velocities(num_bins_, vec3(0));

    Particle p;
    for (int i = 0; i < n; i++) {
        p = initial_particles_[i];
        positions[i] = p.position;
        velocities[i] = p.velocity;
        densities[i] = p.density;
    }

    // Create particle buffers on GPU and copy data into the first buffer.
    position_buffer_ = gl::Ssbo::create(n * sizeof(vec3), positions.data(), GL_STATIC_DRAW);
    velocity_buffer_ = gl::Ssbo::create(n * sizeof(vec3), velocities.data(), GL_STATIC_DRAW);
    density_buffer_ = gl::Ssbo::create(n * sizeof(float), densities.data(), GL_STATIC_DRAW);
    pressure_buffer_ = gl::Ssbo::create(n * sizeof(float), pressures.data(), GL_STATIC_DRAW);
    bin_velocity_buffer_ = gl::Ssbo::create(num_bins_ * sizeof(vec3), bin_velocities.data(), GL_STATIC_DRAW);

    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });

    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
    attributes_ = gl::Vao::create();
}

/**
 * Compile bin velocity compute shader
 */
void Fluid::compileBinVelocityProg() {
    OutputDebugStringA("\tcompiling fluid bin_velocity shader\n");

    gl::ScopedBuffer scoped_velocity_ssbo(velocity_buffer_);
    velocity_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    sort_->offsetBuffer()->bindBase(1);

    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    sort_->countBuffer()->bindBase(2);

    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    sort_->sortedBuffer()->bindBase(3);

    gl::ScopedBuffer scoped_bin_velocity_ssbo(bin_velocity_buffer_);
    bin_velocity_buffer_->bindBase(4);

    bin_velocity_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/binVelocity.comp")));
}

/**
 * Compile density compute shader
 */
void Fluid::compileDensityProg() {
    OutputDebugStringA("\tcompiling fluid density shader\n");

    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_bucket_ssbo(sort_->bucketBuffer());
    sort_->bucketBuffer()->bindBase(1);

    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    sort_->offsetBuffer()->bindBase(2);

    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    sort_->countBuffer()->bindBase(3);

    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    sort_->sortedBuffer()->bindBase(4);

    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    density_buffer_->bindBase(5);

    gl::ScopedBuffer scoped_pressure_ssbo(pressure_buffer_);
    pressure_buffer_->bindBase(6);

    density_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/density.comp")));
}

/**
 * Compile update compute shader
 */
void Fluid::compileUpdateProg() {
    OutputDebugStringA("\tcompiling fluid update shader\n");

    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_bucket_ssbo(sort_->bucketBuffer());
    sort_->bucketBuffer()->bindBase(1);

    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    sort_->offsetBuffer()->bindBase(2);

    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    sort_->countBuffer()->bindBase(3);

    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    sort_->sortedBuffer()->bindBase(4);

    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    density_buffer_->bindBase(5);

    gl::ScopedBuffer scoped_pressure_ssbo(pressure_buffer_);
    pressure_buffer_->bindBase(6);

    gl::ScopedBuffer scoped_velocity_ssbo(velocity_buffer_);
    velocity_buffer_->bindBase(7);

    gl::ScopedBuffer scoped_bin_velocity_ssbo(bin_velocity_buffer_);
    bin_velocity_buffer_->bindBase(8);

    update_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/update.comp")));
}

/**
 * Compil render shader
 */
void Fluid::compileRenderProg() {
    OutputDebugStringA("\tcompiling fluid render shader\n");

    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

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
    num_partitions_ = int(glm::ceil(float(num_particles_) / float(CHUNK_SIZE)));
    num_bins_ = int(pow(grid_res_, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_ = bin_size_;

    generateInitialParticles();

    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_work_groups_);

    OutputDebugStringA("creating buffers\n");
    prepareBuffers();

    OutputDebugStringA("initializing sorter\n");
    sort_ = Sort::create()
                ->numItems(num_particles_)
                ->numBins(num_bins_)
                ->gridRes(grid_res_)
                ->binSize(bin_size_)
                ->positionBuffer(position_buffer_);
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
    gl::ScopedBuffer scoped_velocity_ssbo(velocity_buffer_);
    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    gl::ScopedBuffer scoped_bin_velocity_ssbo(bin_velocity_buffer_);
    runProg(num_bins_);
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg() {
    gl::ScopedGlslProg prog(density_prog_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("mass", particle_mass_);
    update_prog_->uniform("kernel", kernel_);
    update_prog_->uniform("weightConstant", weight_constant_);
    update_prog_->uniform("k", k_);
    update_prog_->uniform("restDensity", rest_density_);
    update_prog_->uniform("restPressure", rest_pressure_);
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedBuffer scoped_bucket_ssbo(sort_->bucketBuffer());
    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    gl::ScopedBuffer scoped_pressure_ssbo(pressure_buffer_);
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
    update_prog_->uniform("particleCount", num_particles_);
    update_prog_->uniform("gravity", gravity_);
    update_prog_->uniform("mass", particle_mass_);
    update_prog_->uniform("kernel", kernel_);
    update_prog_->uniform("viscosityCoefficient", viscosity_coefficient_);
    update_prog_->uniform("viscosityWeight", viscosity_weight_);
    update_prog_->uniform("pressureWeight", pressure_weight_);
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedBuffer scoped_bucket_ssbo(sort_->bucketBuffer());
    gl::ScopedBuffer scoped_offset_ssbo(sort_->offsetBuffer());
    gl::ScopedBuffer scoped_count_ssbo(sort_->countBuffer());
    gl::ScopedBuffer scoped_sorted_ssbo(sort_->sortedBuffer());
    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    gl::ScopedBuffer scoped_pressure_ssbo(pressure_buffer_);
    runProg();
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    sort_->run();
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
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedVao vao(attributes_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

void Fluid::reset() {}

/**
 * Create a new fluid simulation with a smart pointer
 */
FluidRef Fluid::create(const std::string& name) { return std::make_shared<Fluid>(name); }
