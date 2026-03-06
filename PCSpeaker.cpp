#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// WS2812 is on GP16 per board definition
#define WS2812_PIN      PICO_DEFAULT_WS2812_PIN
#define WS2812_FREQ_HZ  800000

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

// Colour helper - WS2812 is GRB order
static inline uint32_t colour_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint sm  = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, WS2812_FREQ_HZ);

    const uint32_t RED   = colour_grb(32, 0, 0);   // Dimmed - full brightness is eyewatering
    const uint32_t GREEN = colour_grb(0, 32, 0);

    bool toggle = false;

    while (true) {
        put_pixel(pio, sm, toggle ? GREEN : RED);
        toggle = !toggle;
        sleep_ms(2000);
    }
}