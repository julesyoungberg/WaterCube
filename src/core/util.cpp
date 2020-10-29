#include "./Util.h"

using namespace core;

void util::log(char* format, ...) {
    va_list vl;

    //  format is the last argument specified; all
    //  others must be accessed using the variable-argument macros.
    va_start(vl, format);
    static char message[1024];
    static char message2[1024];

    vsprintf(message, format, vl);
    va_end(vl);

    int len = (int)strlen(message);
    // assert( len < 1024); // Exceeded internal print buffer.
    if (len >= 1024) {
        strncpy(message2, message, 1022);
        strcpy(message, message2);
    } else {
        message[len] = '\n';
    }

    OutputDebugStringA(message);
}
