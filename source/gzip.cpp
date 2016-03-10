#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "gzip.h"

// load a gz stream from disk
uint8_t* gzOpen(const char* path, int* size)
{
    // buffer granularity
    const int c_bufGran = 1024;
    // allocate and initial buffer
    uint8_t* buffer = (uint8_t*)malloc(c_bufGran);
    //
    gzFile file = gzopen(path, "rb");
    if (file == nullptr)
        return nullptr;
    //
    int tsize = 0;
    while (true) {
        // read a grain from the compression stream
        int ret = gzread(file, buffer + tsize, c_bufGran);
        // error
        if (ret == -1) {
            free(buffer);
            buffer = nullptr;
            break;
        }
        // end of the file if less then grain size
        if (ret < c_bufGran)
            break;
        // resize by a grain
        buffer = (uint8_t*)realloc(buffer, c_bufGran + (tsize += c_bufGran));
    }
    //
    gzclose(file);
    // report the output size
    if (size != nullptr)
        *size = tsize;
    //
    return (uint8_t*)buffer;
}

// free a previously allocated gz stream
void gzClose(uint8_t* data)
{
    if (data != nullptr)
        free(data);
}
