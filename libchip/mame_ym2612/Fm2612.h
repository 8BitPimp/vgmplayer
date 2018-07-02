#pragma once

#include "Fm.h"

void* ym2612_init(int baseclock, int rate);
void ym2612_shutdown(void* chip);
void ym2612_reset(void* chip);
void ym2612_render(void* chip, int* buffer, int length, bool add);
int ym2612_write(void *chip, int a, UINT8 v);
void ym2612_set_mute(void* chip, int mute);
float ym2612_get_channel_volume(void* chip, int chn);
const char* ym2612_about(void);
int ym2612_write(void* chip, int port, int a, UINT8 v);
