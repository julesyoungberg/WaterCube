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
void Fluid::generateInitialParticles() {
    OutputDebugStringA("creating particles\n");
    int n = num_particles_;
    initial_particles_.assign(n, Particle());

    const float azimuth =
        256.0f * static_cast<float>(M_PI) / initial_particles_.size();
    const float inclination =
        static_cast<float>(M_PI) / initial_particles_.size();
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

        auto& p = initial_particles_.at(i);
        p.position = center + vec3(x, y, z);
        p.velocity = Rand::randVec3() * 10.0f; // random initial velocity
    }
}

/**
 * Prepares shared memory buffers
 * TODO: add bucket, distance function, wall weight function buffers
 */
void Fluid::prepareBuffers() {
    int n = num_particles_;
    std::vector<vec3> positions(n, vec3());
    std::vector<vec3> velocities(n, vec3());
    std::vector<float> densities(n, 1);

    Particle p;
    for (int i = 0; i < n; i++) {
        p = initial_particles_[i];
        positions[i] = p.position;
        velocities[i] = p.velocity;
        densities[i] = p.density;
    }

    // Create particle buffers on GPU and copy data into the first buffer.
    position_buffer_ =
        gl::Ssbo::create(n * sizeof(vec3), positions.data(), GL_STATIC_DRAW);

    velocity_buffer_ =
        gl::Ssbo::create(n * sizeof(vec3), velocities.data(), GL_STATIC_DRAW);

    density_buffer_ =
        gl::Ssbo::create(n * sizeof(float), densities.data(), GL_STATIC_DRAW);

    std::vector<GLuint> ids(num_particles_);
    GLuint curr_id = 0;
    std::generate(ids.begin(), ids.end(),
                  [&curr_id]() -> GLuint { return curr_id++; });

    ids_vbo_ = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
    attributes_ = gl::Vao::create();
}

void Fluid::compileBucketProg() {
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    bucket_prog_ = gl::GlslProg::create(
        gl::GlslProg::Format().compute(loadAsset("bucketGeneration.comp")));
}

void Fluid::compileDensityProg() {
    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    density_buffer_->bindBase(0);

    density_prog_ = gl::GlslProg::create(
        gl::GlslProg::Format().compute(loadAsset("densityComputation.comp")));
}

void Fluid::compileUpdateProg() {
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    gl::ScopedBuffer scoped_velocity_ssbo(velocity_buffer_);
    velocity_buffer_->bindBase(1);

    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    density_buffer_->bindBase(2);

    update_prog_ = gl::GlslProg::create(
        gl::GlslProg::Format().compute(loadAsset("particleUpdate.comp")));
}

void Fluid::compileRenderProg() {
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    position_buffer_->bindBase(0);

    render_prog_ =
        gl::GlslProg::create(gl::GlslProg::Format()
                                 .vertex(loadAsset("particleRender.vert"))
                                 .fragment(loadAsset("particleRender.frag"))
                                 .attribLocation("particleId", 0));
}

/**
 * Compiles and prepares shader programs
 * TODO: handle error
 */
void Fluid::compileShaders() {
    OutputDebugStringA("compiling glsl");

    gl::ScopedVao vao(attributes_);
    gl::ScopedBuffer scopedIds(ids_vbo_);
    gl::enableVertexAttribArray(0);
    gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

    compileRenderProg();
    compileBucketProg();
    compileDensityProg();
    compileUpdateProg();
}

FluidRef Fluid::setup() {
    OutputDebugStringA("creating fluid\n");

    generateInitialParticles();

    OutputDebugStringA("creating shared buffer\n");
    ivec3 count = gl::getMaxComputeWorkGroupCount();
    CI_ASSERT(count.x >= num_particles_ / WORK_GROUP_SIZE);

    prepareBuffers();
    compileShaders();

    OutputDebugStringA("fluid created\n");
    return std::make_shared<Fluid>(*this);
}

void Fluid::runProg() {
    gl::dispatchCompute(num_particles_ / WORK_GROUP_SIZE, 1, 1);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::runBucketProg() {
    gl::ScopedGlslProg prog(bucket_prog_);
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    runProg();
}

void Fluid::runDensityProg() {
    gl::ScopedGlslProg prog(density_prog_);
    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    runProg();
}

void Fluid::runUpdateProg() {
    gl::ScopedGlslProg prog(update_prog_);
    // update_prog_->uniform("uTime", (float)getElapsedSeconds());
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedBuffer scoped_velocity_ssbo(velocity_buffer_);
    gl::ScopedBuffer scoped_density_ssbo(density_buffer_);
    runProg();
}

void Fluid::update(double time) {
    runBucketProg();
    runDensityProg();
    runUpdateProg();
}

void Fluid::draw() {
    gl::enableDepthRead();
    gl::enableDepthWrite();

    gl::ScopedGlslProg render(render_prog_);
    gl::ScopedBuffer scoped_position_ssbo(position_buffer_);
    gl::ScopedVao vao(attributes_);

    gl::context()->setDefaultShaderVars();
    gl::drawArrays(GL_POINTS, 0, num_particles_);
}

void Fluid::reset() {}

FluidRef Fluid::create(const std::string& name, ContainerRef container) {
    return std::make_shared<Fluid>(name, container);
}
