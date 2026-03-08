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
#include "settings.h"
#include "ws2812.pio.h"
#include "ui.h"

// -------------------------------------------------------
// Pin definitions
// -------------------------------------------------------
#define WS2812_PIN      PICO_DEFAULT_WS2812_PIN
#define WS2812_FREQ_HZ  800000
#define ENC_A           2
#define ENC_B           3
#define ENC_PB          4

// -------------------------------------------------------
// State machine
// -------------------------------------------------------
typedef enum {
    STATE_OFF = 0,
    STATE_POWERING_ON,
    STATE_VOLUME,
    STATE_INPUT,
    STATE_BASS,
    STATE_MID,
    STATE_TREBLE,
    STATE_BALANCE,
    STATE_POWERING_OFF
} app_state_t;

// -------------------------------------------------------
// WS2812
// -------------------------------------------------------
static PIO  _pio;
static uint _sm;

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(_pio, _sm, pixel_grb << 8u);
}

static inline uint32_t colour_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

// -------------------------------------------------------
// LVGL tick callback
// -------------------------------------------------------
static bool lvgl_tick_cb(repeating_timer_t *rt) {
    lv_tick_inc(1);
    return true;
}

// -------------------------------------------------------
// Apply all TDA7439 settings from struct
// -------------------------------------------------------
static void apply_settings(const settings_t *s, uint8_t volume) {
    tda7439_set_input(s->input);
    tda7439_set_volume(volume);
    tda7439_set_bass(s->bass);
    tda7439_set_mid(s->mid);
    tda7439_set_treble(s->treble);
    tda7439_set_balance(s->balance);
}

// -------------------------------------------------------
// WS2812 fade helper - simple linear fade between two colours over a duration in ms
// -------------------------------------------------------
static uint8_t _led_r = 0, _led_g = 32, _led_b = 0;  // Start red

static void ws2812_set(uint8_t r, uint8_t g, uint8_t b) {
    _led_r = r; _led_g = g; _led_b = b;
    put_pixel(colour_grb(r, g, b));
}

static void ws2812_fade(uint8_t r2, uint8_t g2, uint8_t b2,
                        uint32_t duration_ms) {
    const uint32_t steps   = 50;
    const uint32_t step_ms = duration_ms / steps;

    for (uint32_t i = 0; i <= steps; i++) {
        uint8_t r = (uint8_t)((int)_led_r + ((int)r2 - (int)_led_r) * (int)i / (int)steps);
        uint8_t g = (uint8_t)((int)_led_g + ((int)g2 - (int)_led_g) * (int)i / (int)steps);
        uint8_t b = (uint8_t)((int)_led_b + ((int)b2 - (int)_led_b) * (int)i / (int)steps);
        put_pixel(colour_grb(r, g, b));
        sleep_ms(step_ms);
    }
    _led_r = r2; _led_g = g2; _led_b = b2;
}

// -------------------------------------------------------
// Centralised state entry - owns all display + LED updates
// -------------------------------------------------------
static void on_state_enter(app_state_t state, const settings_t *s, uint8_t volume) {
    switch (state) {
        case STATE_OFF:
            ws2812_set(0, 32, 0);    // Red = off
            lv_obj_clean(lv_scr_act());
            ui_init();                           // Black screen
            break;
        case STATE_VOLUME:
            ws2812_set(32, 0, 0);    // Green
            ui_show_volume(volume);
            break;
        case STATE_INPUT:
            ws2812_set(0, 0, 32);    // Blue
            ui_show_input(s->input);
            break;
        case STATE_BASS:
            ws2812_set(20, 15, 0);   // Amber
            ui_show_bass(s->bass);
            break;
        case STATE_MID:
            ws2812_set(20, 15, 0);   // Amber
            ui_show_mid(s->mid);
            break;
        case STATE_TREBLE:
            ws2812_set(20, 15, 0);   // Amber
            ui_show_treble(s->treble);
            break;
        case STATE_BALANCE:
            ws2812_set(20, 15, 0);   // Amber
            ui_show_balance(s->balance);
            break;
        default:
            break;
    }
}

// -------------------------------------------------------
// Main
// -------------------------------------------------------
int main() {
    stdio_init_all();

    // WS2812
    _pio = pio0;
    _sm  = 0;
    uint offset = pio_add_program(_pio, &ws2812_program);
    ws2812_program_init(_pio, _sm, offset, WS2812_PIN, WS2812_FREQ_HZ);
    put_pixel(colour_grb(0, 32, 0));    // Red = off

    // Hardware init
    relay_init();
    gc9a01_init();
    gc9a01_lvgl_init();
    ui_init();
    encoder_init(ENC_A, ENC_B, ENC_PB);

    // LVGL tick
    static repeating_timer_t lvgl_tick_timer;
    add_repeating_timer_ms(1, lvgl_tick_cb, NULL, &lvgl_tick_timer);

    // Load settings from flash
    settings_t settings;
    settings_load(&settings);

    // Volume lives in RAM only - starts muted on cold boot
    uint8_t cur_volume = 48;

    app_state_t state          = STATE_OFF;
    uint32_t last_activity_ms  = 0;
    uint32_t power_off_time_ms = 0;

    while (true) {
        int8_t delta   = encoder_get_delta();
        bool   pressed = encoder_button_pressed();
        bool   held    = encoder_button_held();

        // Reset activity timer on any input
        if (delta != 0 || pressed || held) {
            last_activity_ms = to_ms_since_boot(get_absolute_time());
        }

        // Auto-return to volume after 5 seconds of inactivity
        if (state > STATE_POWERING_ON &&
            state != STATE_VOLUME &&
            state != STATE_POWERING_OFF) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if ((now - last_activity_ms) > 5000) {
                state = STATE_VOLUME;
                on_state_enter(state, &settings, cur_volume);
                last_activity_ms = now;
                printf("Timeout - returning to volume\n");
            }
        }

        switch (state) {

            // -------------------------------------------
            case STATE_OFF:
                if (pressed) {
                    uint32_t now = to_ms_since_boot(get_absolute_time());
                    if ((now - power_off_time_ms) > 500) {
                        state = STATE_POWERING_ON;
                        printf("Powering on...\n");
                    }
                }
                break;

            // -------------------------------------------
            case STATE_POWERING_ON:
                relay_power_on();
                ws2812_fade(32, 0, 0, 1000);   // Fade to green over 1 second
                if (tda7439_init()) {
                    apply_settings(&settings, cur_volume);
                    state = STATE_VOLUME;
                    on_state_enter(state, &settings, cur_volume);
                    printf("Power on OK\n");
                } else {
                    relay_power_off();
                    state = STATE_OFF;
                    on_state_enter(state, &settings, cur_volume);
                    printf("TDA7439 init failed\n");
                }
                break;

            // -------------------------------------------
            case STATE_VOLUME:
                if (delta != 0) {
                    int16_t v = (int16_t)cur_volume - delta;
                    if (v < TDA7439_VOL_MIN) v = TDA7439_VOL_MIN;
                    if (v > TDA7439_VOL_MAX) v = TDA7439_VOL_MAX;
                    cur_volume = (uint8_t)v;
                    tda7439_set_volume(cur_volume);
                    ui_show_volume(cur_volume);
                }
                if (pressed) {
                    state = STATE_INPUT;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_INPUT:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.input + delta;
                    if (v < TDA7439_INPUT_MIN) v = TDA7439_INPUT_MIN;
                    if (v > TDA7439_INPUT_MAX) v = TDA7439_INPUT_MAX;
                    settings.input = (uint8_t)v;
                    tda7439_set_input(settings.input);
                    ui_show_input(settings.input);
                }
                if (pressed) {
                    state = STATE_BASS;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_BASS:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.bass + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.bass = (uint8_t)v;
                    tda7439_set_bass(settings.bass);
                    ui_show_bass(settings.bass);
                }
                if (pressed) {
                    state = STATE_MID;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_MID:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.mid + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.mid = (uint8_t)v;
                    tda7439_set_mid(settings.mid);
                    ui_show_mid(settings.mid);
                }
                if (pressed) {
                    state = STATE_TREBLE;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_TREBLE:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.treble + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.treble = (uint8_t)v;
                    tda7439_set_treble(settings.treble);
                    ui_show_treble(settings.treble);
                }
                if (pressed) {
                    state = STATE_BALANCE;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_BALANCE:
                if (delta != 0) {
                    int8_t v = settings.balance + delta;
                    if (v < TDA7439_BAL_MIN) v = TDA7439_BAL_MIN;
                    if (v > TDA7439_BAL_MAX) v = TDA7439_BAL_MAX;
                    settings.balance = v;
                    tda7439_set_balance(settings.balance);
                    ui_show_balance(settings.balance);
                }
                if (pressed) {
                    settings_save(&settings);
                    printf("Settings saved\n");
                    state = STATE_VOLUME;
                    on_state_enter(state, &settings, cur_volume);
                }
                if (held) {
                    state = STATE_POWERING_OFF;
                }
                break;

            // -------------------------------------------
            case STATE_POWERING_OFF:
                tda7439_mute();
                relay_power_off();
                ws2812_fade(0, 32, 0, 500);     // Fade to red
                lv_obj_clean(lv_scr_act());
                ui_init();                       // Black screen
                power_off_time_ms = to_ms_since_boot(get_absolute_time());
                encoder_button_pressed();
                state = STATE_OFF;
                printf("Power off\n");
                break;
        }

        lv_timer_handler();
        sleep_ms(5);
    }
}