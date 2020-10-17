#include "./Fluid.h"

using namespace core;

Fluid::Fluid(const std::string& name, ContainerRef container)
    : BaseObject(name), container_(container) {}

Fluid::~Fluid() {}

FluidRef Fluid::numParticles(int n) {
    num_particles_ = n;
    return std::make_shared<Fluid>(*this);
}

/**
 * Generates a vector of particles used as the simulations initial state
 */
std::vector<Particle> Fluid::generateParticles() {
    OutputDebugStringA("creating particles\n");
    int n = num_particles_;
    std::vector<Particle> particles;
    particles.assign(n, Particle());

    const float azimuth = 256.0f * static_cast<float>(M_PI) / particles.size();
    const float inclination = static_cast<float>(M_PI) / particles.size();
    const float radius = 180.0f;
    vec3 center = vec3(getWindowCenter() + vec2(0.0f, 40.0f), 0.0f);

    for (int i = 0; i < n; i++) {
        // const float x = ((std::rand() % n) / (float)n);
        // const float y = ((std::rand() % n) / (float)n);
        // const float z = ((std::rand() % n) / (float)n);
        // auto& p = particles.at(i);
        // p.position = vec3(x, y, z);

        // assign starting values to particles.
        float x = radius * math<float>::sin(inclination * i) *
                  math<float>::cos(azimuth * i);
        float y = radius * math<float>::cos(inclination * i);
        float z = radius * math<float>::sin(inclination * i) *
                  math<float>::sin(azimuth * i);

        auto& p = particles.at(i);
        p.position = center + vec3(x, y, z);
        p.velocity = Rand::randVec3() * 10.0f; // random initial velocity
    }

    return particles;
}

/**
 * Prepares shared memory buffers
 */
void Fluid::prepareBuffers(std::vector<Particle> particles) {
    // Create particle buffers on GPU and copy data into the first buffer.
    // Mark as static since we only write from the CPU once.
    particle_buffer_ = gl::Ssbo::create(particles.size() * sizeof(Particle),
                                        particles.data(), GL_STATIC_DRAW);

    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(),
                  [&curr_id]() -> GLuint { return curr_id++; });

    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
    attributes_ = gl::Vao::create();
}

/**
 * Compiles and prepares shader programs
 * TODO: handle error
 */
void Fluid::compileShaders() {
    OutputDebugStringA("compiling glsl");

    gl::ScopedBuffer scoped_particle_ssbo(particle_buffer_);
    particle_buffer_->bindBase(0);

    render_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("particleRender.vert"))
                                 .fragment(loadAsset("particleRender.frag"))
                                 .attribLocation("particleId", 0));

    gl::ScopedVao vao(attributes_);
    gl::ScopedBuffer scopedIds(ids_vbo_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

    update_prog_ = gl::GlslProg::create(
        gl::GlslProg::Format().compute(loadAsset("particleUpdate.comp")));
}

FluidRef Fluid::setup() {
    OutputDebugStringA("creating fluid\n");

    auto particles = generateParticles();

    OutputDebugStringA("creating shared buffer\n");
    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_particles_ / WORK_GROUP_SIZE);

    prepareBuffers(particles);
    compileShaders();

    OutputDebugStringA("fluid created\n");
    return std::make_shared<Fluid>(*this);
}

void Fluid::update(double time) {
    gl::ScopedGlslProg prog(update_prog_);
    // update_prog_->uniform("uTime", (float)getElapsedSeconds());

    gl::ScopedBuffer scoped_particle_ssbo(particle_buffer_);

    gl::dispatchCompute(num_particles_ / WORK_GROUP_SIZE, 1, 1);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedBuffer scoped_particle_ssbo(particle_buffer_);
    gl::ScopedVao vao(attributes_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

void Fluid::reset() {}

FluidRef Fluid::create(const std::string& name, ContainerRef container) {
    return std::make_shared<Fluid>(name, container);
}
