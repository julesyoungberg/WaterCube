#pragma once

#include "./Util.h"

using namespace core;

void util::log(char* format, ...) {
    va_list vl;

    //  format is the last argument specified; all
    //  others must be accessed using the variable-argument macros.
    va_start(vl, format);
    static char message[1024];
    for (int i = 0; i < 1024; i++) {
        message[i] = '\0';
    }

    vsprintf(message, format, vl);
    va_end(vl);

    int len = (int)strlen(message);
    message[len] = '\n';

    OutputDebugStringA(message);
}

/**
 * Generic run shader on particles - one for each
 */
void util::runProg(ivec3 work_groups) {
    gl::dispatchCompute(work_groups.x, work_groups.y, work_groups.z);
    gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void util::runProg(int work_groups) { runProg(ivec3(work_groups, 1, 1)); }

gl::GlslProgRef util::compileComputeShader(char* filename) {
    return gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset(filename)));
}
