#include "./Fluid.h"

#include <glm/gtx/quaternion.hpp>
#include <time.h>

using namespace core;

Fluid::Fluid(const std::string& name) : BaseObject(name), position_(0), rotation_(0, 0, 0, 0) {
    size_ = 1.0f;
    num_particles_ = 1000;
    grid_res_ = 2;
    gravity_strength_ = 900.0f;
    gravity_direction_ = vec3(0, -1, 0);
    particle_radius_ = 0.2f; // 0.01f;
    kernel_radius_ = particle_radius_ * 4.0f;
    rest_density_ = 1000.0f;
    particle_mass_ = particle_radius_ * 8.0f;
    viscosity_coefficient_ = 0.0101f;
    stiffness_ = 30.0f;
    rest_pressure_ = 3;
    render_mode_ = 3;
    point_scale_ = 75.0f; // 650.0f;
    dt_ = 0.0003f;        // 0.0008f;
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

FluidRef Fluid::gravityStrength(float g) {
    gravity_strength_ = g;
    return thisRef();
}

FluidRef Fluid::sortInterval(int i) {
    sort_interval_ = i;
    return thisRef();
}

/**
 * setup GUI configuration parameters
 */
void Fluid::addParams(params::InterfaceGlRef p) {
    // p->addParam("Number of Particles", &num_particles_, "min=100 step=100");
    p->addParam("Render Mode", &render_mode_, "min=0 max=9 step=1");
    // p->addParam("Grid Resolution", &grid_res_, "min=1 max=1000 step=1");
    // p->addParam("Particle Mass", &particle_mass_, "min=0.0001 max=2.0 step=0.001");
    // p->addParam("Viscosity", &viscosity_coefficient_, "min=0.001 max=2.0 step=0.001");
    p->addParam("Stifness", &stiffness_, "min=0.0 max=500.0 step=1.0");
    p->addParam("Rest Density", &rest_density_, "min=0.0 max=1000.0 step=1.0");
    p->addParam("Rest Pressure", &rest_pressure_, "min=0.0 max=10.0 step=0.1");
    p->addParam("Gravity Strength", &gravity_strength_, "min=-10.0 max=10.0 step=0.1");
}

/**
 * set rotation and update settings
 */
void Fluid::setRotation(quat r) {
    rotation_ = r;
    mat4 rotation_matrix = glm::toMat4(r);
    vec4 rotated = rotation_matrix * vec4(0, -1, 0, 1);
    gravity_direction_ = vec3(rotated);
}

/**
 * set camera position and update children
 */
void Fluid::setCameraPosition(vec3 p) {
    camera_position_ = p - vec3(size_ / 2.0f);
    marching_cube_->setCameraPosition(camera_position_);
}

/**
 * set light position and update children
 */
void Fluid::setLightPosition(vec3 p) {
    light_position_ = p;
    marching_cube_->setLightPosition(p);
}

float getRand() { return (float)rand() / RAND_MAX; }

/**
 * Generates a vector of particles used as the simulations initial state
 */
void Fluid::generateInitialParticles() {
    // srand((unsigned)time(NULL));
    srand(0);

    util::log("creating particles");
    int n = num_particles_;
    initial_particles_.assign(n, Particle());
    float distance = std::cbrt((4.0f * pow(particle_radius_, 3) * M_PI) / (3.0f * n));
    int d = int(ceil(std::cbrt(n)));

    for (int z = 0; z < d; z++) {
        for (int y = 0; y < d; y++) {
            for (int x = 0; x < d; x++) {
                int index = z * d * d + y * d + x;
                if (index >= n) {
                    break;
                }

                initial_particles_[index].position =
                    vec3(getRand() * size_, getRand() * size_, getRand() * size_);
                // initial_particles_[index].position = vec3(x, y, z) * distance;
                // initial_particles_[index].position +=
                //     vec3(getRand() * 0.1 - 0.05, getRand() * 0.1 - 0.05, getRand() * 0.1 - 0.05);
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
    util::log("size: %f, numBins: %d, binSize: %f, kernelRadius: %f", size_, num_bins_, bin_size_,
              kernel_radius_);

    // Part of equation (8) from Harada
    pressure_weight_ = static_cast<float>(45.0f / (M_PI * pow(kernel_radius_, 6)));
    // Part of equation (9) from Harada
    viscosity_weight_ = static_cast<float>(45.0f / (M_PI * pow(kernel_radius_, 6)));
    // Part of equation (10) from Harada
    kernel_weight_ = static_cast<float>(315.0f / (64.0f * M_PI * pow(kernel_radius_, 9)));

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
                         ->subdivisions(5);
    marching_cube_->setup();

    util::log("fluid created");
    return std::make_shared<Fluid>(*this);
}

/**
 * Run density compute shader
 */
void Fluid::runDensityProg(GLuint particle_buffer) {
    gl::ScopedGlslProg prog(density_prog_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sort_->getCountBuffer());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sort_->getOffsetBuffer());

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

    update_prog_->uniform("size", size_);
    update_prog_->uniform("binSize", bin_size_);
    update_prog_->uniform("gridRes", grid_res_);
    update_prog_->uniform("dt", dt_); // time_step);
    update_prog_->uniform("numParticles", num_particles_);
    update_prog_->uniform("gravity", gravity_direction_ * gravity_strength_);
    update_prog_->uniform("particleMass", particle_mass_);
    update_prog_->uniform("kernelRadius", kernel_radius_);
    update_prog_->uniform("viscosityCoefficient", viscosity_coefficient_);
    update_prog_->uniform("viscosityWeight", viscosity_weight_);
    update_prog_->uniform("pressureWeight", pressure_weight_);
    update_prog_->uniform("particleRadius", particle_radius_);

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
    advect_prog_->uniform("dt", dt_); // time_step);
    advect_prog_->uniform("numParticles", num_particles_);

    runProg();
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

/**
 * Update simulation logic - run compute shaders
 */
void Fluid::update(double time) {
    odd_frame_ = !odd_frame_;
    GLuint in_particles = odd_frame_ ? particle_buffer1_ : particle_buffer2_;
    GLuint out_particles = odd_frame_ ? particle_buffer2_ : particle_buffer1_;

    util::printParticles(in_particles, 10);

    sort_->run(in_particles, out_particles);

    runDensityProg(out_particles);
    runUpdateProg(out_particles, float(time));
    // runAdvectProg(out_particles, float(time));

    util::printParticles(out_particles, 10);

    // marching_cube_->update(out_particles, sort_->getCountBuffer(), sort_->getOffsetBuffer());
}

/**
 * render particles
 */
void Fluid::renderParticles() {
    const float pointRadius = particle_radius_ * point_scale_;
    gl::pointSize(pointRadius * 2.0f);

    gl::ScopedGlslProg render(render_particles_prog_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                     odd_frame_ ? particle_buffer2_ : particle_buffer1_);
    glBindVertexArray(odd_frame_ ? vao2_ : vao1_);

    render_particles_prog_->uniform("renderMode", render_mode_);
    render_particles_prog_->uniform("size", size_);
    render_particles_prog_->uniform("binSize", bin_size_);
    render_particles_prog_->uniform("gridRes", grid_res_);
    render_particles_prog_->uniform("lightPos", light_position_);
    render_particles_prog_->uniform("cameraPos", camera_position_);

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
    vec3 offset = vec3(-size_ / 2.0f);
    gl::translate(offset.x, offset.y, offset.z);

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

    gl::popMatrices();
}
