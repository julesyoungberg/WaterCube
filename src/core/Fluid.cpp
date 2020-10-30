#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name)
    : BaseObject(name), size_(1.0f), num_particles_(1000), grid_res_(5), position_(0), gravity_(-9.8f),
      particle_mass_(0.02f), kernel_radius_(0.1828f), viscosity_coefficient_(0.035f), stiffness_(250.0f),
      rest_density_(998.27f), rest_pressure_(0) {}

Fluid::~Fluid() {}

FluidRef Fluid::numParticles(int n) {
    num_particles_ = n;
    return thisRef();
}

FluidRef Fluid::gridRes(int r) {
    grid_res_ = r;
    return thisRef();
}

FluidRef Fluid::size(float s) {
    size_ = s;
    return thisRef();
}

FluidRef Fluid::kernelRadius(float r) {
    kernel_radius_ = r;
    return thisRef();
}

FluidRef Fluid::particleMass(float m) {
    particle_mass_ = m;
    return thisRef();
}

FluidRef Fluid::viscosityCoefficient(float c) {
    viscosity_coefficient_ = c;
    return thisRef();
}

FluidRef Fluid::viscosityWeight(float w) {
    viscosity_weight_ = w;
    return thisRef();
}

FluidRef Fluid::pressureWeight(float w) {
    pressure_weight_ = w;
    return thisRef();
}

FluidRef Fluid::kernelWeight(float w) {
    kernel_weight_ = w;
    return thisRef();
}

FluidRef Fluid::stiffness(float s) {
    stiffness_ = s;
    return thisRef();
}

FluidRef Fluid::restDensity(float d) {
    rest_density_ = d;
    return thisRef();
}

FluidRef Fluid::restPressure(float p) {
    rest_pressure_ = p;
    return thisRef();
}

FluidRef Fluid::position(vec3 p) {
    position_ = p;
    return thisRef();
}

FluidRef Fluid::gravity(float g) {
    gravity_ = g;
    return thisRef();
}

void Fluid::addParams(params::InterfaceGlRef p) {
    p->addParam("Number of Particles", &num_particles_, "min=100 step=100");
    p->addParam("Grid Resolution", &grid_res_, "min=1 max=1000 step=1");
    p->addParam("Particle Mass", &particle_mass_, "min=0.001 max=2.0 step=0.001");
    p->addParam("Viscosity", &viscosity_coefficient_, "min=0.001 max=2.0 step=0.001");
    p->addParam("Stifness", &stiffness_, "min=0.0 max=500.0 step=1.0");
    p->addParam("Rest Density", &rest_density_, "min=0.0 max=1000.0 step=1.0");
    p->addParam("Rest Pressure", &rest_pressure_, "min=0.0 max=10.0 step=0.1");
    p->addParam("Gravity", &gravity_, "min=-10.0 max=10.0 step=0.1");
}

/**
 * Generates a vector of particles used as the simulations initial state
 */
void Fluid::generateInitialParticles() {
    util::log("creating particles");
    int n = num_particles_;
    initial_particles_.assign(n, Particle());

    for (auto& p : initial_particles_) {
        p.position = (Rand::randVec3() * 0.5f + 0.5f) * size_;
        p.velocity = Rand::randVec3() * 0.1f;
    }
}

/**
 * Prepares shared memory buffers
 * TODO: add distance function, wall weight function buffers
 */
void Fluid::prepareBuffers() {
    util::log("preparing fluid buffers");

    int n = num_particles_;

    // Create particle buffers on GPU and copy data into the first buffer.
    particle_buffer_1_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);
    particle_buffer_2_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);

    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });

    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
    attributes_ = gl::Vao::create();

    auto texture_format = gl::Texture3d::Format().internalFormat(GL_RGBA32F);
    velocity_field_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, texture_format);
}

/**
 * Compile bin velocity compute shader
 */
void Fluid::compileBinVelocityProg() {
    util::log("\tcompiling fluid bin_velocity shader");
    bin_velocity_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/binVelocity.comp")));
}

/**
 * Compile density compute shader
 */
void Fluid::compileDensityProg() {
    util::log("\tcompiling fluid density shader");
    density_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/density.comp")));
}

/**
 * Compile update compute shader
 */
void Fluid::compileUpdateProg() {
    util::log("\tcompiling fluid update shader");
    update_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/update.comp")));
}

/**
 * Compil render shader
 */
void Fluid::compileRenderProg() {
    util::log("\tcompiling fluid render shader");
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
    util::log("compiling fluid shaders");

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
    util::log("initializing fluid");
    num_work_groups_ = ceil(num_particles_ / float(WORK_GROUP_SIZE));
    num_bins_ = int(pow(grid_res_, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_radius_ = bin_size_;

    viscosity_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    pressure_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    kernel_weight_ = static_cast<float>(315.0f / (64.0f * M_PI * std::pow(kernel_radius_, 9)));

    container_ = Container::create("fluidContainer", size_);

    generateInitialParticles();

    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_work_groups_);

    util::log("creating buffers");
    prepareBuffers();

    util::log("initializing sorter");
    sort_ = Sort::create()->numItems(num_particles_)->numBins(num_bins_)->gridRes(grid_res_)->binSize(bin_size_);
    sort_->prepareBuffers();

    sort_->compileShaders();
    compileShaders();

    util::log("fluid created");
    return std::make_shared<Fluid>(*this);
}

/**
 * Generic run shader on particles - one for each
 */
void Fluid::runProg(ivec3 work_groups) {
    gl::dispatchCompute(work_groups.x, work_groups.y, work_groups.z);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::runProg(int work_groups) { runProg(ivec3(work_groups, 1, 1)); }

void Fluid::runProg() { runProg(num_work_groups_); }

/**
 * Run bin velocity compute shader
 */
void Fluid::runBinVelocityProg() {
    gl::ScopedGlslProg prog(bin_velocity_prog_);
    bin_velocity_prog_->uniform("countGrid", 0);
    bin_velocity_prog_->uniform("offsetGrid", 1);
    bin_velocity_prog_->uniform("velocityField", 2);
    bin_velocity_prog_->uniform("gridRes", grid_res_);
    runProg(num_bins_);
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg() {
    gl::ScopedGlslProg prog(density_prog_);
    bin_velocity_prog_->uniform("countGrid", 0);
    bin_velocity_prog_->uniform("offsetGrid", 1);
    density_prog_->uniform("binSize", bin_size_);
    density_prog_->uniform("gridRes", grid_res_);
    density_prog_->uniform("numParticles", num_particles_);
    density_prog_->uniform("particleMass", particle_mass_);
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
    bin_velocity_prog_->uniform("countGrid", 0);
    bin_velocity_prog_->uniform("offsetGrid", 1);
    update_prog_->uniform("velocityField", 2);
    update_prog_->uniform("size", size_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("dt", time_step);
    update_prog_->uniform("numParticles", num_particles_);
    update_prog_->uniform("gravity", vec3(0, gravity_, 0));
    update_prog_->uniform("particleMass", particle_mass_);
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

    sort_->getCountGrid()->bind(0);
    sort_->getOffsetGrid()->bind(1);

    sort_->run();

    velocity_field_->bind(3);

    runBinVelocityProg();
    runDensityProg();
    runUpdateProg(time);

    sort_->getCountGrid()->unbind(0);
    sort_->getOffsetGrid()->unbind(1);
    velocity_field_->unbind(3);
}

/**
 * Draw simulation logic
 */
void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::pushMatrices();
    float half = size_ / 2.0f;
    vec3 offset = vec3(-half, -half, -half);
    gl::translate(offset.x, offset.y, offset.z);

    container_->draw();

    gl::ScopedBuffer scoped_particle_buffer1(particle_buffer_1_);
    gl::ScopedBuffer scoped_particle_buffer2(particle_buffer_2_);
    particle_buffer_1_->bindBase(odd_frame_ ? 0 : 1);
    particle_buffer_2_->bindBase(odd_frame_ ? 1 : 0);

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedVao vao(attributes_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
    gl::popMatrices();
}

void Fluid::reset() {}
