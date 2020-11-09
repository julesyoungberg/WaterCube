#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name)
    : BaseObject(name), size_(1.0f), num_particles_(1000), grid_res_(7), position_(0), gravity_(0), // -9.8f),
      particle_mass_(0.02f), kernel_radius_(0.1828f), viscosity_coefficient_(0.035f), stiffness_(250.0f),
      rest_density_(998.27f), rest_pressure_(0), render_mode_(4), particle_radius_(0.0457) {}

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

FluidRef Fluid::particleRadius(float r) {
    particle_radius_ = r;
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

FluidRef Fluid::renderMode(int m) {
    render_mode_ = m;
    return thisRef();
}

FluidRef Fluid::gravity(float g) {
    gravity_ = g;
    return thisRef();
}

void Fluid::addParams(params::InterfaceGlRef p) {
    p->addParam("Number of Particles", &num_particles_, "min=100 step=100");
    p->addParam("Render Mode", &render_mode_, "min=0 max=4 step=1");
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

void Fluid::generateBoundaryPlanes() {
    util::log("creating boundaries");

    std::vector<std::vector<ivec3>> walls = {
        {ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(1, 1, 0), ivec3(0, 1, 0)},
        {ivec3(0, 0, 1), ivec3(1, 0, 1), ivec3(1, 1, 1), ivec3(0, 1, 1)},
        {ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(1, 1, 1), ivec3(0, 1, 1)},
        {ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(1, 0, 1), ivec3(0, 0, 1)},
        {ivec3(0, 0, 0), ivec3(0, 1, 0), ivec3(0, 1, 1), ivec3(0, 0, 1)},
        {ivec3(1, 0, 0), ivec3(1, 1, 0), ivec3(1, 1, 1), ivec3(1, 0, 1)},
    };

    boundaries_.resize(walls.size());

    for (int i = 0; i < walls.size(); i++) {
        auto const wall = walls[i];

        Plane p = Plane();

        vec3 ab = wall[1] - wall[0];
        vec3 ac = wall[3] - wall[0];
        vec3 n = cross(ab, ac);

        p.normal = normalize(n) * float(pow(-1, i));
        p.point = wall[2];

        util::log("\t%d - normal: <%f, %f, %f>, point: <%f, %f, %f>", i, p.normal.x, p.normal.y, p.normal.z, p.point.x,
                  p.point.y, p.point.z);

        boundaries_[i] = p;
    }
}

void Fluid::prepareWallWeightFunction() {
    util::log("\tpreparing wall weight function");

    int divisions = 10;
    float step = kernel_radius_ / float(divisions);
    std::vector<float> values(divisions + 1, 0);

    util::log("\t\tgenerating values");

    for (int i = 0; i <= divisions; i++) {
        float dist = i * step;
        values[i] = kernel_weight_ * pow(kernel_radius_ * kernel_radius_ - dist * dist, 3);
    }

    util::log("\t\tcreating buffer");

    auto texture_format = gl::Texture1d::Format().internalFormat(GL_R32F);
    wall_weight_function_ = gl::Texture1d::create(values.data(), GL_R32F, divisions + 1, texture_format);
}

/**
 * Prepares shared memory buffers
 */
void Fluid::prepareBuffers() {
    util::log("preparing fluid buffers");

    int n = num_particles_;

    // Create particle buffers on GPU and copy data into the first buffer.
    util::log("\tcreating particle buffers");
    particle_buffer1_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);
    particle_buffer2_ = gl::Ssbo::create(n * sizeof(Particle), initial_particles_.data(), GL_STATIC_DRAW);
    util::log("\tcreating boundary buffer");
    boundary_buffer_ = gl::Ssbo::create(n * sizeof(Plane), boundaries_.data(), GL_STATIC_DRAW);

    util::log("\tcreating ids vbo");
    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating attributes vao");
    attributes1_ = gl::Vao::create();
    attributes2_ = gl::Vao::create();

    util::log("\tcreating velocity field");
    auto texture_format = gl::Texture3d::Format().internalFormat(GL_RGBA32F);
    velocity_field_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, texture_format);
    util::log("\tcreating disance field");
    distance_field_ = gl::Texture3d::create(grid_res_ + 1, grid_res_ + 1, grid_res_ + 1, texture_format);

    prepareWallWeightFunction();
}

void Fluid::compileDistanceFieldProg() {
    util::log("\tcompiling fluid distanceField compute shader");
    distance_field_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/distanceField.comp")));
}

/**
 * Compile bin velocity compute shader
 */
void Fluid::compileBinVelocityProg() {
    util::log("\tcompiling fluid binVelocity compute shader");
    bin_velocity_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/binVelocity.comp")));
}

/**
 * Compile density compute shader
 */
void Fluid::compileDensityProg() {
    util::log("\tcompiling fluid density compute shader");
    density_prog_ = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("fluid/density.comp")));
}

/**
 * Compile update compute shader
 */
void Fluid::compileUpdateProg() {
    util::log("\tcompiling fluid update compute shader");
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
 */
void Fluid::compileShaders() {
    util::log("compiling fluid shaders");

    gl::ScopedBuffer scopedIds(ids_vbo_);
    gl::ScopedVao vao1(attributes1_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
    gl::ScopedVao vao2(attributes2_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

    compileDistanceFieldProg();
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
    num_work_groups_ = int(ceil(float(num_particles_) / float(WORK_GROUP_SIZE)));
    num_bins_ = int(pow(grid_res_, 3));
    distance_field_size_ = int(pow(grid_res_ + 1, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_radius_ = bin_size_;

    // Parti of equation (8) from Harada
    pressure_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    // Part of equation (9) from Harada
    viscosity_weight_ = static_cast<float>(45.0f / (M_PI * std::pow(kernel_radius_, 6)));
    // Part of equation (10) from Harada
    kernel_weight_ = static_cast<float>(315.0f / (64.0f * M_PI * std::pow(kernel_radius_, 9)));

    container_ = Container::create("fluidContainer", size_);

    generateInitialParticles();
    generateBoundaryPlanes();

    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_work_groups_);

    util::log("creating buffers");
    prepareBuffers();

    util::log("initializing sorter");
    sort_ = Sort::create()->numItems(num_particles_)->gridRes(grid_res_)->binSize(bin_size_);
    sort_->prepareBuffers();

    sort_->compileShaders();
    compileShaders();

    runDistanceFieldProg();

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

void Fluid::runDistanceFieldProg() {
    gl::ScopedBuffer scoped_boundary_buffer(boundary_buffer_);
    boundary_buffer_->bindBase(0);
    distance_field_->bind(0);

    gl::ScopedGlslProg prog(distance_field_prog_);

    distance_field_prog_->uniform("distanceField", 0);
    distance_field_prog_->uniform("gridRes", grid_res_ + 1);
    distance_field_prog_->uniform("numBoundaries", int(boundaries_.size()));
    distance_field_prog_->uniform("binSize", bin_size_);

    runProg(ivec3(int(ceil(distance_field_size_ / 4.0f))));

    distance_field_->bind(0);
    boundary_buffer_->unbindBase();
}

/**
 * Run bin velocity compute shader
 */
void Fluid::runBinVelocityProg() {
    gl::ScopedGlslProg prog(bin_velocity_prog_);
    bin_velocity_prog_->uniform("countGrid", 0);
    bin_velocity_prog_->uniform("offsetGrid", 1);
    bin_velocity_prog_->uniform("velocityField", 2);
    bin_velocity_prog_->uniform("gridRes", grid_res_);
    runProg(ivec3(int(ceil(num_bins_ / 4.0f))));
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg() {
    gl::ScopedGlslProg prog(density_prog_);
    density_prog_->uniform("countGrid", 0);
    density_prog_->uniform("offsetGrid", 1);
    density_prog_->uniform("distanceField", 3);
    density_prog_->uniform("wallWeight", 4);
    density_prog_->uniform("size", size_);
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
    update_prog_->uniform("countGrid", 0);
    update_prog_->uniform("offsetGrid", 1);
    update_prog_->uniform("velocityField", 2);
    update_prog_->uniform("distanceField", 3);
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
    gl::ScopedBuffer scoped_particle_buffer1(particle_buffer1_);
    gl::ScopedBuffer scoped_particle_buffer2(particle_buffer2_);
    particle_buffer1_->bindBase(odd_frame_ ? 0 : 1);
    particle_buffer2_->bindBase(odd_frame_ ? 1 : 0);

    sort_->getCountGrid()->bind(0);
    sort_->getOffsetGrid()->bind(1);

    sort_->run();

    velocity_field_->bind(2);
    distance_field_->bind(3);
    wall_weight_function_->bind(4);

    runBinVelocityProg();
    runDensityProg();
    runUpdateProg(float(time));

    sort_->getCountGrid()->unbind(0);
    sort_->getOffsetGrid()->unbind(1);
    velocity_field_->unbind(2);
    distance_field_->bind(3);
    wall_weight_function_->unbind(4);
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

    gl::ScopedBuffer scoped_particle_buffer(odd_frame_ ? particle_buffer2_ : particle_buffer1_);
    if (odd_frame_) {
        particle_buffer1_->bindBase(1);
    } else {
        particle_buffer1_->bindBase(0);
    }

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedVao vao(odd_frame_ ? attributes1_ : attributes2_);

    render_prog_->uniform("renderMode", render_mode_);
    render_prog_->uniform("binSize", bin_size_);
    render_prog_->uniform("gridRes", grid_res_);
    const float pointRadius = particle_radius_ * (render_mode_ ? 6.0f : 3.5f);
    render_prog_->uniform("pointRadius", pointRadius);
    render_prog_->uniform("pointScale", 650.0f);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
    gl::popMatrices();
}

void Fluid::reset() {}
