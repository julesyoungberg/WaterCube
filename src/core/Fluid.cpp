#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name) : BaseObject(name) {
    size_ = 1.0f;
    num_particles_ = 1000;
    grid_res_ = 8;
    position_ = vec3(0);
    gravity_ = 0; // -0.05f;
    particle_mass_ = 0.05f;
    kernel_radius_ = 0.1828f;
    viscosity_coefficient_ = 0.035f;
    stiffness_ = 250.0f;
    rest_pressure_ = 0;
    render_mode_ = 5;
    particle_radius_ = 0.05f; // 0.0457f;
    rest_density_ = 4;        // 998.27f
}

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

        if (p.position.x < 0 || p.position.y < 0 || p.position.z < 0 || p.position.z > size_ ||
            p.position.y > size_ || p.position.z > size_) {
            util::log("ERROR: <%f, %f, %f>", p.position.x, p.position.y, p.position.z);
        }
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

        vec3 normal = normalize(n) * float(pow(-1, i));
        vec3 point = wall[2];
        p.normal = vec4(normal, 1);
        p.point = vec4(point, 1);

        util::log("\t%d - normal: <%f, %f, %f>, point: <%f, %f, %f>", i, p.normal.x, p.normal.y,
                  p.normal.z, p.point.x, p.point.y, p.point.z);

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
    wall_weight_function_ =
        gl::Texture1d::create(values.data(), GL_R32F, divisions + 1, texture_format);
}

void Fluid::prepareParticleBuffers() {
    util::log("\tcreating particle buffers");

    const auto size = num_particles_ * sizeof(Particle);

    // Buffer 1
    glCreateBuffers(1, &particle_buffer1_);
    glNamedBufferStorage(particle_buffer1_, size, initial_particles_.data(), 0);
    glCreateVertexArrays(1, &vao1_);
    glEnableVertexArrayAttrib(vao1_, 0);
    glVertexArrayVertexBuffer(vao1_, 0, particle_buffer1_, 0, sizeof(Particle));
    glVertexArrayAttribBinding(vao1_, 0, 0);
    glVertexArrayAttribFormat(vao1_, 0, 3, GL_FLOAT, GL_FALSE, 0);

    // Buffer 2
    glCreateBuffers(1, &particle_buffer2_);
    glNamedBufferStorage(particle_buffer2_, size, initial_particles_.data(), 0);
    glCreateVertexArrays(1, &vao2_);
    glEnableVertexArrayAttrib(vao2_, 0);
    glVertexArrayVertexBuffer(vao2_, 0, particle_buffer2_, 0, sizeof(Particle));
    glVertexArrayAttribBinding(vao2_, 0, 0);
    glVertexArrayAttribFormat(vao2_, 0, 3, GL_FLOAT, GL_FALSE, 0);
}

void Fluid::prepareGridParticles() {
    util::log("\tcreating grid particles");

    // generate particles
    int n = int(pow(grid_res_, 3));
    grid_particles_.resize(n);
    for (int z = 0; z < grid_res_; z++) {
        for (int y = 0; y < grid_res_; y++) {
            for (int x = 0; x < grid_res_; x++) {
                grid_particles_.push_back(ivec4(x, y, z, 1));
            }
        }
    }

    // create buffer
    glCreateBuffers(1, &grid_buffer_);
    glNamedBufferStorage(grid_buffer_, n * sizeof(ivec4), grid_particles_.data(), 0);
    glCreateVertexArrays(1, &grid_vao_);
    glEnableVertexArrayAttrib(grid_vao_, 0);
    glVertexArrayVertexBuffer(grid_vao_, 0, grid_buffer_, 0, sizeof(ivec4));
    glVertexArrayAttribBinding(grid_vao_, 0, 0);
    glVertexArrayAttribFormat(grid_vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
}

/**
 * Prepares shared memory buffers
 */
void Fluid::prepareBuffers() {
    util::log("preparing fluid buffers");

    util::log("\tcreating boundary buffer - boundaries: %d", boundaries_.size());
    boundary_buffer_ =
        gl::Ssbo::create(boundaries_.size() * sizeof(Plane), boundaries_.data(), GL_STATIC_DRAW);

    util::log("\tcreating velocity field");
    auto texture3d_format = gl::Texture3d::Format().internalFormat(GL_RGBA32F);
    velocity_field_ = gl::Texture3d::create(grid_res_, grid_res_, grid_res_, texture3d_format);
    util::log("\tcreating disance field");
    distance_field_ =
        gl::Texture3d::create(grid_res_ + 1, grid_res_ + 1, grid_res_ + 1, texture3d_format);

    prepareParticleBuffers();
    prepareWallWeightFunction();
    prepareGridParticles();
}

/**
 * Compiles and prepares shader programs
 */
void Fluid::compileShaders() {
    util::log("compiling fluid shaders");

    util::log("\tcompiling fluid distanceField compute shader");
    distance_field_prog_ = util::compileComputeShader("fluid/distanceField.comp");

    util::log("\tcompiling fluid binVelocity compute shader");
    bin_velocity_prog_ = util::compileComputeShader("fluid/binVelocity.comp");

    util::log("\tcompiling fluid density compute shader");
    density_prog_ = util::compileComputeShader("fluid/density.comp");

    util::log("\tcompiling fluid update compute shader");
    update_prog_ = util::compileComputeShader("fluid/update.comp");

    util::log("\tcompiling fluid geometry shader");
    geometry_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                              .vertex(loadAsset("fluid/particle.vert"))
                                              .fragment(loadAsset("fluid/particle.frag")));

    util::log("\tcompiling render grid shader");
    render_grid_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                                 .vertex(loadAsset("fluid/grid.vert"))
                                                 .fragment(loadAsset("fluid/grid.frag")));
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
    util::log("size: %f, numBins: %d, binSize: %f, kernelRadius: %f", size_, num_bins_, bin_size_,
              kernel_radius_);

    // Part of equation (8) from Harada
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

    prepareBuffers();
    compileShaders();

    util::log("initializing sorter");
    sort_ = Sort::create()->numItems(num_particles_)->gridRes(grid_res_)->binSize(bin_size_);
    sort_->prepareBuffers();
    sort_->compileShaders();

    util::log("initializing marching cube");
    marching_cube_ = MarchingCube::create()->size(size_);
    marching_cube_->setup(8);

    runDistanceFieldProg();

    util::log("fluid created");
    return std::make_shared<Fluid>(*this);
}

void Fluid::runProg() { util::runProg(num_work_groups_); }

void Fluid::runDistanceFieldProg() {
    gl::ScopedGlslProg prog(distance_field_prog_);

    gl::ScopedBuffer scoped_boundary_buffer(boundary_buffer_);
    boundary_buffer_->bindBase(0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glUniform1i(glGetUniformLocation(distance_field_prog_->getHandle(), "distanceField"), 1);
    glBindImageTexture(1, distance_field_->getId(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    distance_field_prog_->uniform("gridRes", grid_res_ + 1);
    distance_field_prog_->uniform("numBoundaries", int(boundaries_.size()));
    distance_field_prog_->uniform("binSize", bin_size_);

    util::runProg(ivec3(ceil(distance_field_size_ / 4.0f)));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    boundary_buffer_->unbindBase();
}

/**
 * Run bin velocity compute shader
 */
void Fluid::runBinVelocityProg(gl::SsboRef particles) {
    gl::ScopedGlslProg prog(bin_velocity_prog_);

    gl::ScopedBuffer scoped_particles(particles);
    particles->bindBase(0);

    // sort_->getCountGrid()->bind(1);
    // sort_->getOffsetGrid()->bind(2);
    // bin_velocity_prog_->uniform("countGrid", 1);
    // bin_velocity_prog_->uniform("offsetGrid", 2);

    glActiveTexture(GL_TEXTURE0 + 3);
    glUniform1i(glGetUniformLocation(bin_velocity_prog_->getHandle(), "velocityField"), 3);
    glBindImageTexture(3, velocity_field_->getId(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    bin_velocity_prog_->uniform("gridRes", grid_res_);

    util::runProg(ivec3(ceil(num_bins_ / 4.0f)));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    // sort_->getCountGrid()->unbind(1);
    // sort_->getOffsetGrid()->unbind(2);

    particles->unbindBase();
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg(gl::SsboRef particles) {
    gl::ScopedGlslProg prog(density_prog_);

    gl::ScopedBuffer scoped_particles(particles);
    particles->bindBase(0);

    // sort_->getCountGrid()->bind(1);
    // sort_->getOffsetGrid()->bind(2);
    // distance_field_->bind(3);
    // wall_weight_function_->bind(4);

    density_prog_->uniform("countGrid", 1);
    density_prog_->uniform("offsetGrid", 2);
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
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

    // sort_->getCountGrid()->unbind(1);
    // sort_->getOffsetGrid()->unbind(2);
    // distance_field_->unbind(3);
    // wall_weight_function_->unbind(4);

    particles->unbindBase();
}

/**
 * Run update compute shader
 */
void Fluid::runUpdateProg(gl::SsboRef particles, float time_step) {
    gl::ScopedGlslProg prog(update_prog_);

    gl::ScopedBuffer scoped_particles(particles);
    particles->bindBase(0);

    // sort_->getCountGrid()->bind(1);
    // sort_->getOffsetGrid()->bind(2);
    // velocity_field_->bind(3);
    // distance_field_->bind(4);

    update_prog_->uniform("countGrid", 1);
    update_prog_->uniform("offsetGrid", 2);
    update_prog_->uniform("velocityField", 3);
    update_prog_->uniform("distanceField", 4);
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
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // sort_->getCountGrid()->unbind(1);
    // sort_->getOffsetGrid()->unbind(2);
    // velocity_field_->unbind(3);
    // distance_field_->unbind(4);

    particles->unbindBase();
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    odd_frame_ = !odd_frame_;
    GLuint in_particles = odd_frame_ ? particle_buffer1_ : particle_buffer2_;
    GLuint out_particles = odd_frame_ ? particle_buffer2_ : particle_buffer1_;

    sort_->run(in_particles, out_particles);

    // runBinVelocityProg(out_particles);

    // auto velocities = util::getVecs(velocity_field_, num_bins_);
    // util::log("bin velocities: ");
    // for (int i = 0; i < 100; i++) {
    //     vec3 v = velocities[i];
    //     util::log("<%f, %f, %f>", v.x, v.y, v.z);
    // }

    // runDensityProg(out_particles);
    // runUpdateProg(out_particles, float(time));

    // if (render_mode_ == 7) {
    //     marching_cube_->update(out_particles, num_particles_, 0.5f);
    // }
}

void Fluid::renderGeometry() {
    gl::ScopedGlslProg render(geometry_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     odd_frame_ ? particle_buffer2_ : particle_buffer1_);
    glBindVertexArray(odd_frame_ ? vao2_ : vao1_);

    geometry_prog_->uniform("renderMode", render_mode_);
    geometry_prog_->uniform("size", size_);
    geometry_prog_->uniform("binSize", bin_size_);
    geometry_prog_->uniform("gridRes", grid_res_);
    const float pointRadius = particle_radius_ * (render_mode_ ? 6.0f : 3.5f);
    geometry_prog_->uniform("pointRadius", pointRadius);
    geometry_prog_->uniform("pointScale", 650.0f);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

void Fluid::renderGrid() {
    gl::pointSize(10);

    gl::ScopedGlslProg render(render_grid_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, grid_buffer_);
    glBindVertexArray(grid_vao_);

    velocity_field_->bind(1);

    render_grid_prog_->uniform("tex", 1);
    render_grid_prog_->uniform("binSize", bin_size_);
    render_grid_prog_->uniform("gridRes", grid_res_);
    render_grid_prog_->uniform("size", size_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, grid_particles_.size());

    velocity_field_->unbind(1);
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

    if (render_mode_ == 5) {
        sort_->renderGrid(size_);
    } else if (render_mode_ == 6) {
        renderGrid();
    } else if (render_mode_ == 7) {
        marching_cube_->renderDensity();
    } else if (render_mode_ == 8) {
        marching_cube_->render();
    } else {
        renderGeometry();
    }

    // auto particles = util::getParticles(particle_buffer1_id_, num_particles_);
    // util::log("particles");
    // for (auto const& p : particles) {
    //     gl::pointSize(2);
    //     gl::color(p.velocity.x, p.velocity.y, p.velocity.z);
    //     gl::begin(GL_POINTS);
    //     gl::vertex(p.position.x, p.position.y, p.position.z);
    //     gl::end();

    //     if (p.position.x < 0 || p.position.y < 0 || p.position.z < 0 || p.position.x > size_ ||
    //     p.position.y > size_
    //     ||
    //         p.position.z > size_) {
    //         util::log("ERROR: <%f, %f, %f>", p.position.x, p.position.y, p.position.z);
    //     }
    // }

    container_->draw();

    gl::popMatrices();
}

void Fluid::reset() {}
