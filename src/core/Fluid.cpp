#include "./Fluid.h"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <time.h>

using namespace core;

Fluid::Fluid(const std::string& name) : BaseObject(name), position_(0), rotation_(0, 0, 0, 0) {
    size_ = 1.0f;
    position_ = -vec3(size_ / 2.0f);
    num_particles_ = 20000;
    // must be less than (size / kernel_radius - b) where b is a positive int
    grid_res_ = 1;
    gravity_strength_ = 900.0f;
    gravity_direction_ = vec3(0, -1, 0);
    particle_radius_ = 0.01f;
    rest_density_ = 1000.0f;
    viscosity_coefficient_ = 20;
    stiffness_ = 100.0f;
    rest_pressure_ = 0.0f;
    render_mode_ = 0;
    point_scale_ = 300.0f;
    time_scale_ = 0.013f;
    rotate_gravity_ = false;
    createParams();
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

FluidRef Fluid::particleRadius(float r) {
    particle_radius_ = r;
    return thisRef();
}

FluidRef Fluid::viscosityCoefficient(float c) {
    viscosity_coefficient_ = c;
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

FluidRef Fluid::gravityStrength(float g) {
    gravity_strength_ = g;
    return thisRef();
}

/**
 * setup GUI configuration parameters
 */
void Fluid::createParams() {
    params_ = params::InterfaceGl::create("WaterCube", ivec2(225, 200));
    params_->addParam("Rotation", &rotation_);
    params_->addParam("Render Mode", &render_mode_, "min=0 max=9 step=1");
    params_->addParam("Viscosity", &viscosity_coefficient_, "min=0.0 max=1000.0 step=10.0");
    params_->addParam("Stifness", &stiffness_, "min=0.0 max=500.0 step=10.0");
    params_->addParam("Rest Density", &rest_density_, "min=0.0 max=2000.0 step=100.0");
    params_->addParam("Rest Pressure", &rest_pressure_, "min=0.0 max=100000.0 step=100.0");
    params_->addParam("Gravity Strength", &gravity_strength_, "min=0.0 max=1000.0 step=10.0");
    params_->addParam("Rotate Gravity", &rotate_gravity_);
}

/**
 * Generates a vector of particles used as the simulations initial state
 */
void Fluid::generateInitialParticles() {
    srand(0); // (unsigned)time(NULL));

    util::log("creating particles");
    int n = num_particles_;
    initial_particles_.assign(n, Particle());
    float distance = particle_radius_ * 2.0f;
    int d = int(ceil(std::cbrt(n)));

    float jitter = distance * 0.5f;
    auto getJitter = [jitter]() { return ((float)rand() / RAND_MAX) * jitter - (jitter / 2.0f); };

    for (int z = 0; z < d; z++) {
        for (int y = 0; y < d; y++) {
            for (int x = 0; x < d; x++) {
                int index = z * d * d + y * d + x;
                if (index >= n) {
                    break;
                }

                // random position
                // initial_particles_[index].position =
                //     vec3(getRand() * size_, getRand() * size_, getRand() * size_);

                // dam break
                initial_particles_[index].position = vec3(x, y, z) * distance;
                initial_particles_[index].position += vec3(getJitter(), getJitter(), getJitter());
            }
        }
    }
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

    // debug buffer
    glCreateBuffers(1, &debug_buffer_);
    std::vector<uint32_t> zeros(num_particles_, 0);
    glNamedBufferStorage(debug_buffer_, num_particles_ * sizeof(uint32_t), zeros.data(), 0);
}

/**
 * Prepares shared memory buffers
 */
void Fluid::prepareBuffers() {
    util::log("preparing fluid buffers");

    prepareParticleBuffers();

    gl::memoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT);
}

/**
 * Compiles and prepares shader programs
 */
void Fluid::compileShaders() {
    util::log("compiling fluid shaders");

    util::log("\tcompiling fluid density compute shader");
    density_prog_ = util::compileComputeShader("fluid/density.comp");

    util::log("\tcompiling fluid update compute shader");
    update_prog_ = util::compileComputeShader("fluid/update.comp");

    util::log("\tcompiling fluid advect compute shader");
    advect_prog_ = util::compileComputeShader("fluid/advect.comp");

    util::log("\tcompiling fluid particles shader");
    render_particles_prog_ = gl::GlslProg::create(gl::GlslProg::Format()
                                                      .vertex(loadAsset("fluid/particle.vert"))
                                                      .fragment(loadAsset("fluid/particle.frag")));
}

/**
 * setup simulation - initialize buffers and shaders
 */
FluidRef Fluid::setup() {
    util::log("initializing fluid");
    first_frame_ = true;
    num_work_groups_ = int(ceil(float(num_particles_) / float(WORK_GROUP_SIZE)));
    num_bins_ = int(pow(grid_res_, 3));
    bin_size_ = size_ / float(grid_res_);
    kernel_radius_ = particle_radius_ * 4.0f;
    particle_mass_ = particle_radius_ * 8.0f;
    util::log("size: %f, numBins: %d, binSize: %f, kernelRadius: %f, particleMass: %f", size_,
              num_bins_, bin_size_, kernel_radius_, particle_mass_);

    poly6_kernel_const_ = static_cast<float>(315.0 / (64.0 * M_PI * glm::pow(kernel_radius_, 9)));
    spiky_kernel_const_ = static_cast<float>(-45.0 / (M_PI * glm::pow(kernel_radius_, 6)));
    viscosity_kernel_const_ = static_cast<float>(45.0 / (M_PI * glm::pow(kernel_radius_, 6)));

    container_ = Container::create("fluidContainer", size_);

    generateInitialParticles();

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
                         ->threshold(0.5f)
                         ->sortingResolution(grid_res_)
                         ->subdivisions(1);
    marching_cube_->setup();

    util::log("fluid created");
    return std::make_shared<Fluid>(*this);
}

vec3 Fluid::translateWorldSpacePosition(vec3 p) { return p - position_; }

vec3 Fluid::rotateWorldSpacePosition(vec3 p) {
    mat3 rotation_matrix = glm::transpose(glm::toMat3(rotation_));
    return rotation_matrix * p;
}

vec3 Fluid::getRelativePosition(vec3 p) {
    return translateWorldSpacePosition(rotateWorldSpacePosition(p));
}

vec3 Fluid::getRelativeCameraPosition() { return getRelativePosition(camera_position_); }

vec3 Fluid::getRelativeLightPosition() { return getRelativePosition(light_position_); }

Ray Fluid::getRelativeMouseRay() {
    mat4 matrix = glm::translate(-position_);
    Ray ray = mouse_ray_;
    ray.transform(matrix);
    return ray;
}

void Fluid::updateGravity() {
    if (rotate_gravity_) {
        gravity_direction_ = rotateWorldSpacePosition(vec3(0, -1, 0));
    }
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg(GLuint particle_buffer) {
    gl::ScopedGlslProg prog(density_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sort_->getCountBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sort_->getOffsetBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, debug_buffer_);

    density_prog_->uniform("size", size_);
    density_prog_->uniform("binSize", bin_size_);
    density_prog_->uniform("gridRes", grid_res_);
    density_prog_->uniform("numParticles", num_particles_);
    density_prog_->uniform("particleMass", particle_mass_);
    density_prog_->uniform("kernelRadius", kernel_radius_);
    density_prog_->uniform("stiffness", stiffness_);
    density_prog_->uniform("restDensity", rest_density_);
    density_prog_->uniform("restPressure", rest_pressure_);
    density_prog_->uniform("poly6KernelConst", poly6_kernel_const_);

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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, debug_buffer_);

    Ray mouse_ray = getRelativeMouseRay();

    update_prog_->uniform("size", size_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("dt", time_step * time_scale_);
    update_prog_->uniform("numParticles", num_particles_);
    update_prog_->uniform("gravity", gravity_direction_ * gravity_strength_);
    update_prog_->uniform("particleMass", particle_mass_);
    update_prog_->uniform("kernelRadius", kernel_radius_);
    update_prog_->uniform("viscosityCoefficient", viscosity_coefficient_);
    update_prog_->uniform("cameraPosition", mouse_ray.getOrigin());
    update_prog_->uniform("mouseRayDirection", mouse_ray.getDirection());
    update_prog_->uniform("spikyKernelConst", spiky_kernel_const_);
    update_prog_->uniform("viscosityKernelConst", viscosity_kernel_const_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Run advect compute shader
 */
void Fluid::runAdvectProg(GLuint particle_buffer, float time_step) {
    gl::ScopedGlslProg prog(advect_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);

    advect_prog_->uniform("size", size_);
    advect_prog_->uniform("dt", time_step * time_scale_);
    advect_prog_->uniform("numParticles", num_particles_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    updateGravity();

    // odd_frame_ = !odd_frame_;
    // GLuint in_particles = odd_frame_ ? particle_buffer1_ : particle_buffer2_;
    // GLuint out_particles = odd_frame_ ? particle_buffer2_ : particle_buffer1_;

    // util::printParticles(in_particles, debug_buffer_, 10, bin_size_);

    // sort_->run(in_particles, out_particles);

    runDensityProg(particle_buffer1_);
    runUpdateProg(particle_buffer1_, float(time));
    // runAdvectProg(out_particles, float(time));

    // util::printParticles(out_particles, debug_buffer_, 10, bin_size_);

    // marching_cube_->setCameraPosition(getRelativeCameraPosition());
    // marching_cube_->setLightPosition(getRelativeLightPosition());
    // marching_cube_->update(out_particles, sort_->getCountBuffer(), sort_->getOffsetBuffer());
}

/**
 * Draw gravity vector
 */
void Fluid::drawGravity() {
    gl::color(Color("red"));
    gl::lineWidth(2);

    gl::pushMatrices();
    gl::translate(vec3(size_ / 2.0f));

    gl::begin(GL_LINES);
    gl::vertex(vec3(0));
    gl::vertex(gravity_direction_);
    gl::end();

    gl::popMatrices();
}

/**
 * Draw the light for debugging
 */
void Fluid::drawLight() {
    gl::color(Color("red"));
    gl::pointSize(10);

    gl::begin(GL_POINTS);
    gl::vertex(getRelativeLightPosition());
    gl::end();
}

/**
 * render particles
 */
void Fluid::renderParticles() {
    const float pointRadius = particle_radius_ * point_scale_;
    gl::pointSize(pointRadius * 2.0f);

    gl::ScopedGlslProg render(render_particles_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer1_);
    glBindVertexArray(vao1_);

    render_particles_prog_->uniform("renderMode", render_mode_);
    render_particles_prog_->uniform("size", size_);
    render_particles_prog_->uniform("binSize", bin_size_);
    render_particles_prog_->uniform("gridRes", grid_res_);
    render_particles_prog_->uniform("lightPos", light_position_);
    render_particles_prog_->uniform("cameraPos", getRelativeCameraPosition());

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

/**
 * Draw simulation logic
 */
void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();
    gl::pushMatrices();
    gl::rotate(rotation_);
    gl::translate(position_.x, position_.y, position_.z);

    if (render_mode_ == 4) {
        sort_->renderGrid(size_);
    } else if (render_mode_ == 5) {
        marching_cube_->renderParticleGrid();
    } else if (render_mode_ == 6) {
        marching_cube_->renderSurfaceVertices();
    } else if (render_mode_ == 7) {
        marching_cube_->renderSurface();
    } else {
        renderParticles();
    }

    container_->draw();
    // drawGravity();
    // drawLight();

    gl::popMatrices();

    params_->draw();
}
