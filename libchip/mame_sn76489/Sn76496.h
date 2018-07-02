#pragma once

void segapsg_write_stereo(void* chip, unsigned char data);
void segapsg_write_register(void* chip, unsigned char data);
void segapsg_render(void* chip, int* buffer, int samples, bool add);
void segapsg_set_gain(void* chip, int gain);
void* segapsg_init(int base_clock, int rate, bool neg);
void segapsg_shutdown(void* chip);
void segapsg_set_mute(void* chip, int mask);
float segapsg_get_channel_volume(void* chip, int channel);
const char* segapsg_about(void);
