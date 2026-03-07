#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "gc9a01.h"
#include "gc9a01_lvgl.h"
#include "encoder.h"
#include "relay.h"
#include "tda7439.h"
#include "ws2812.pio.h"
#include "hardware/i2c.h"

#define WS2812_PIN      PICO_DEFAULT_WS2812_PIN
#define WS2812_FREQ_HZ  800000

#define ENC_A   2
#define ENC_B   3
#define ENC_PB  4

static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t colour_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

static bool lvgl_tick_cb(repeating_timer_t *rt) {
    lv_tick_inc(1);
    return true;
}

int main() {
    stdio_init_all();

    // WS2812
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, WS2812_FREQ_HZ);
    put_pixel(pio, sm, colour_grb(0, 32, 0));  // Red = off

    // Relays
    relay_init();

    // Display + LVGL
    gc9a01_init();
    gc9a01_lvgl_init();

    // LVGL tick
    static repeating_timer_t lvgl_tick_timer;
    add_repeating_timer_ms(1, lvgl_tick_cb, NULL, &lvgl_tick_timer);

    // Encoder
    encoder_init(ENC_A, ENC_B, ENC_PB);

    // Status label
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Press to power on");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    bool powered = false;

    while (true) {
        if (encoder_button_pressed()) {
            if (!powered) {
                relay_power_on();
                put_pixel(pio, sm, colour_grb(32, 0, 0));  // Green = on
                sleep_ms(1000);  // Give TDA7439 time to power up

                // Initialise TDA7439
                if (tda7439_init()) {
                    tda7439_set_input(1);
                    tda7439_set_volume(20);
                    tda7439_set_bass(7);        // 0dB
                    tda7439_set_mid(7);         // 0dB
                    tda7439_set_treble(7);      // 0dB
                    tda7439_set_balance(0);     // Centre
                    lv_label_set_text(label, "ON - Vol: 20");
                    printf("TDA7439 initialised OK\n");
                } else {
                    lv_label_set_text(label, "TDA7439 ERROR");
                    printf("TDA7439 init failed\n");
                }
            } else {
                tda7439_mute();
                relay_power_off();
                put_pixel(pio, sm, colour_grb(0, 32, 0));  // Red = off
                lv_label_set_text(label, "Press to power on");
                printf("Power OFF\n");
            }
            powered = !powered;
        }

        lv_timer_handler();
        sleep_ms(5);
    }
}