#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

// Default values on first boot or after flash erase
#define SETTINGS_DEFAULT_INPUT    1
#define SETTINGS_DEFAULT_BASS     7   // 0dB
#define SETTINGS_DEFAULT_MID      7   // 0dB
#define SETTINGS_DEFAULT_TREBLE   7   // 0dB
#define SETTINGS_DEFAULT_BALANCE  0   // Centre
#define SETTINGS_DEFAULT_VOLUME   0   // Mute on cold boot

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  input;
    uint8_t  bass;
    uint8_t  mid;
    uint8_t  treble;
    int8_t   balance;
    uint32_t magic;     // Validates that flash has been written
} settings_t;

void     settings_load(settings_t *s);
void     settings_save(const settings_t *s);

#ifdef __cplusplus
}
#endif

#endif