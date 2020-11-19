#include "./Fluid.h"

#include <glm/gtx/string_cast.hpp>

using namespace core;

Fluid::Fluid(const std::string& name) : BaseObject(name) {
    size_ = 1.0f;
    num_particles_ = 1000;
    grid_res_ = 14;
    position_ = vec3(0);
    gravity_ = -900.0f;
    particle_mass_ = 0.1f;
    kernel_radius_ = 0.1828f;
    viscosity_coefficient_ = 0.035f;
    stiffness_ = 250.0f;
    rest_pressure_ = 0;
    render_mode_ = 9;
    particle_radius_ = 0.1f; // 0.0457f;
    rest_density_ = 1400.0f;
    sort_interval_ = 1; // TODO: fix - any value other than 1 results in really shakey movement
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

FluidRef Fluid::sortInterval(int i) {
    sort_interval_ = i;
    return thisRef();
}

FluidRef Fluid::cameraPosition(vec3 p) {
    camera_position_ = p;
    return thisRef();
}

/**
 * setup GUI configuration parameters
 */
void Fluid::addParams(params::InterfaceGlRef p) {
    // p->addParam("Number of Particles", &num_particles_, "min=100 step=100");
    p->addParam("Render Mode", &render_mode_, "min=0 max=9 step=1");
    // p->addParam("Grid Resolution", &grid_res_, "min=1 max=1000 step=1");
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
        p.velocity = Rand::randVec3() * 10.0f;

        if (p.position.x < 0 || p.position.y < 0 || p.position.z < 0 || p.position.z > size_ ||
            p.position.y > size_ || p.position.z > size_) {
            util::log("ERROR: <%f, %f, %f>", p.position.x, p.position.y, p.position.z);
        }
    }
}

/**
 * generates a vector of planes representing the container boundaries
 */
void Fluid::generateBoundaryPlanes() {
    util::log("creating boundaries");

    // walls of the unit cube
    std::vector<std::vector<vec3>> walls = {
        {vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0)},
        {vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1)},
        {vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 1, 1), vec3(0, 1, 1)},
        {vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 0, 1), vec3(0, 0, 1)},
        {vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 1), vec3(0, 0, 1)},
        {vec3(1, 0, 0), vec3(1, 1, 0), vec3(1, 1, 1), vec3(1, 0, 1)},
    };

    for (int i = 0; i < walls.size(); i++) {
        for (int j = 0; j < walls[i].size(); j++) {
            walls[i][j] = walls[i][j] * size_;
        }
    }

    boundaries_.resize(walls.size());

    // compute planes from wall definitions
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

/**
 * generate a 1d map from distance to force weight
 */
void Fluid::prepareWallWeightFunction() {
    util::log("\tpreparing wall weight function");

    // calculate discrete weights for interpolation on the GPU
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

/**
 * prepare main particle buffers
 */
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

/**
 * prepare debugging grid particles
 */
void Fluid::prepareGridParticles() {
    util::log("\tcreating grid particles");
    grid_particles_.resize(int(pow(grid_res_, 3)));
    for (int z = 0; z < grid_res_; z++) {
        for (int y = 0; y < grid_res_; y++) {
            for (int x = 0; x < grid_res_; x++) {
                grid_particles_.push_back(ivec4(x, y, z, 0));
            }
        }
    }
    grid_buffer_ = gl::Ssbo::create(grid_particles_.size() * sizeof(ivec4), grid_particles_.data(),
                                    GL_STATIC_DRAW);

    util::log("\tcreating grid particles ids vbo");
    std::vector<GLuint> ids(grid_particles_.size());
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(), [&curr_id]() -> GLuint { return curr_id++; });
    grid_ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);

    util::log("\tcreating grid particles attributes vao");
    grid_attributes_ = gl::Vao::create();
    gl::ScopedBuffer scopedDids(grid_ids_vbo_);
    gl::ScopedVao grid_vao(grid_attributes_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);
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
                                                 .fragment(loadAsset("fluid/grid.frag"))
                                                 .attribLocation("gridID", 0));
}

/**
 * setup simulation - initialize buffers and shaders
 */
FluidRef Fluid::setup() {
    util::log("initializing fluid");
    first_frame_ = true;
    num_work_groups_ = int(ceil(float(num_particles_) / float(WORK_GROUP_SIZE)));
    num_bins_ = int(pow(grid_res_, 3));
    distance_field_size_ = int(pow(grid_res_ + 1, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_radius_ = bin_size_ / 2.0f;
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
    marching_cube_ = MarchingCube::create()
                         ->size(size_)
                         ->numItems(num_particles_)
                         ->threshold(0.7f)
                         ->sortingResolution(grid_res_)
                         ->subdivisions(6)
                         ->cameraPosition(camera_position_ - vec3(size_ / 2.0f));
    marching_cube_->setup();

    runDistanceFieldProg();

    util::log("fluid created");
    return std::make_shared<Fluid>(*this);
}

/**
 * compute distance field on the GPU
 */
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

    util::runProg(ivec3(int(ceil(distance_field_size_ / 4.0f))));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run bin velocity compute shader
 */
void Fluid::runBinVelocityProg(GLuint particle_buffer) {
    gl::ScopedGlslProg prog(bin_velocity_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sort_->getCountBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sort_->getOffsetBuffer());

    glActiveTexture(GL_TEXTURE0 + 3);
    glUniform1i(glGetUniformLocation(bin_velocity_prog_->getHandle(), "velocityField"), 3);
    glBindImageTexture(3, velocity_field_->getId(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    bin_velocity_prog_->uniform("gridRes", grid_res_);

    util::runProg(ivec3(int(ceil(num_bins_ / 4.0f))));
    gl::memoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg(GLuint particle_buffer) {
    gl::ScopedGlslProg prog(density_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sort_->getCountBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sort_->getOffsetBuffer());

    distance_field_->bind(3);
    wall_weight_function_->bind(4);

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
}

/**
 * Run update compute shader
 */
void Fluid::runUpdateProg(GLuint particle_buffer, float time_step) {
    gl::ScopedGlslProg prog(update_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sort_->getCountBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sort_->getOffsetBuffer());

    distance_field_->bind(3);
    // velocity_field_->bind(4);

    update_prog_->uniform("distanceField", 3);
    // update_prog_->uniform("velocityField", 4);
    update_prog_->uniform("size", size_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("dt", 0.00017f); // time_step);
    update_prog_->uniform("numParticles", num_particles_);
    update_prog_->uniform("gravity", vec3(0, gravity_, 0));
    update_prog_->uniform("particleMass", particle_mass_);
    update_prog_->uniform("kernelRadius", kernel_radius_);
    update_prog_->uniform("viscosityCoefficient", viscosity_coefficient_);
    update_prog_->uniform("viscosityWeight", viscosity_weight_);
    update_prog_->uniform("pressureWeight", pressure_weight_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * print particles for debugging
 */
void Fluid::printParticles(GLuint particle_buffer) {
    const int n = 1;
    std::vector<Particle> particles = util::getParticles(particle_buffer, n);
    util::log("-----Particles-----");
    for (int i = 0; i < n; i++) {
        Particle p = particles[i];
        std::string s = "p=<" + glm::to_string(p.position) + ">";
        s += " v=<" + glm::to_string(p.velocity) + ">";
        s += " d=" + std::to_string(p.density) + ", pr=" + std::to_string(p.pressure);
        util::log("%s", s.c_str());
    }
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    GLuint in_particles = odd_frame_ ? particle_buffer2_ : particle_buffer1_;
    GLuint out_particles = odd_frame_ ? particle_buffer1_ : particle_buffer2_;

    if (first_frame_ || getElapsedFrames() % sort_interval_ == 0) {
        sort_->run(in_particles, out_particles);
        odd_frame_ = !odd_frame_;
        first_frame_ = false;
    }

    // runBinVelocityProg(out_particles);

    runDensityProg(out_particles);
    runUpdateProg(out_particles, float(time));

    // printParticles(out_particles);

    marching_cube_->update(out_particles, sort_->getCountBuffer(), sort_->getOffsetBuffer());
}

/**
 * render particles
 */
void Fluid::renderGeometry() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(geometry_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     odd_frame_ ? particle_buffer1_ : particle_buffer2_);
    glBindVertexArray(odd_frame_ ? vao1_ : vao2_);

    geometry_prog_->uniform("renderMode", render_mode_);
    geometry_prog_->uniform("size", size_);
    geometry_prog_->uniform("binSize", bin_size_);
    geometry_prog_->uniform("gridRes", grid_res_);
    const float pointRadius = particle_radius_ * 6.0f;
    geometry_prog_->uniform("pointRadius", pointRadius);
    geometry_prog_->uniform("pointScale", 650.0f);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

/**
 * render debugging grid
 */
void Fluid::renderGrid() {
    gl::pointSize(5);

    gl::ScopedGlslProg render(render_grid_prog_);
    gl::ScopedVao vao(grid_attributes_);

    gl::ScopedBuffer grid_buffer(grid_buffer_);
    grid_buffer_->bindBase(0);

    velocity_field_->bind(1);

    render_grid_prog_->uniform("tex", 1);
    render_grid_prog_->uniform("binSize", bin_size_);
    render_grid_prog_->uniform("gridRes", grid_res_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, int(grid_particles_.size()));
}

/**
 * Draw simulation logic
 */
void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::pushMatrices();
    vec3 offset = vec3(-size_ / 2.0f);
    gl::translate(offset.x, offset.y, offset.z);

    if (render_mode_ == 5) {
        sort_->renderGrid(size_);
    } else if (render_mode_ == 6) {
        renderGrid();
    } else if (render_mode_ == 7) {
        marching_cube_->renderDensity();
    } else if (render_mode_ == 8) {
        marching_cube_->renderGrid();
    } else if (render_mode_ == 9) {
        marching_cube_->renderSurface();
    } else {
        renderGeometry();
    }

    container_->draw();

    gl::popMatrices();
}
