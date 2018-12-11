#pragma once
#include <cassert>
#include <cstdio>

#include "vgm.h"

struct vgm_fstream_t : public vgm_stream_t {

    vgm_fstream_t(const char* path)
        : _fd(nullptr)
    {
        fopen_s(&_fd, path, "rb");
    }

    bool valid() const
    {
        return _fd != nullptr;
    }

    uint8_t read8() override
    {
        uint8_t out;
        assert(_fd);
        fread(&out, 1, 1, _fd);
        return out;
    }

    uint16_t read16() override
    {
        uint16_t out;
        assert(_fd);
        fread(&out, 1, 2, _fd);
        return out;
    }

    uint32_t read32() override
    {
        uint32_t out;
        assert(_fd);
        fread(&out, 1, 4, _fd);
        return out;
    }

    void read(void* dst, uint32_t size) override
    {
        assert(_fd);
        fread(dst, 1, size, _fd);
    }

    void skip(uint32_t size) override
    {
        assert(_fd);
        fseek(_fd, size, SEEK_CUR);
    }

    void rewind() override {
      ::rewind(_fd);
    }

protected:
    FILE* _fd;
};
