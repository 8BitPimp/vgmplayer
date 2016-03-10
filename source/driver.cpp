#define _CRT_SECURE_NO_WARNINGS
#define _SDL_main_h
#include <SDL/SDL.h>
#include <array>

#include "gzip.h"
#include "vgm.h"

namespace {

void _audio_cb(void* data, uint8_t* stream, int len)
{
    sVGMFile* vgm = (sVGMFile*)data;
    int16_t* smp = (int16_t*)stream;
    len /= sizeof(int16_t);
    vgm_render(vgm, smp, len);
}

bool _startAudio(sVGMFile* file)
{
    if (SDL_Init(SDL_INIT_AUDIO)!=0)
        return false;

    SDL_AudioSpec spec = {
        SAMPLE_RATE,
        AUDIO_S16,
        1,
        0,
        BUFFER_SIZE,
        0,
        0,
        _audio_cb,
        file
    };

    if (SDL_OpenAudio(&spec, nullptr)!=0)
        return false;

    SDL_PauseAudio(0);
    return true;
}

void _stopAudio(void)
{
    SDL_CloseAudio();
    SDL_Quit();
}

bool _writeToFile(sVGMFile* vgm, const char * path) {

    FILE * fd = fopen(path, "wb");
    if (fd) {
        std::array<int16_t, 512> buffer;
        while (!vgm->finished) {
            vgm_render(vgm, &(buffer[0]), buffer.size());
            fwrite(&(buffer[0]), sizeof(uint16_t), buffer.size(), fd);
        }
        fclose(fd);
    }
    return true;
}

} // namespace {}

int main(int argc, const char** args)
{
    const char *path = (argc>1) ? args[1] : "";

    sVGMFile* vgm = vgm_load(path);
    if (vgm == nullptr) {
        printf("unable to load file [%s]\n", path);
        return 1;
    }
    else {
        printf("loaded file [%s]\n", path);
    }

    if (argc>2) {
        _writeToFile(vgm, args[2]);
    }
    else {
        if (_startAudio(vgm)) {
            printf("playing...\n");
            while (!vgm->finished) {
                SDL_Delay(200);
            }
        }
        _stopAudio();
    }

    vgm_free(vgm);
    return 0;
}
