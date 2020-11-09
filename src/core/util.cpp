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
