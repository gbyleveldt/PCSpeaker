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
// WS2812 helpers
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
// Update LVGL label - placeholder until UI is built
// -------------------------------------------------------
static lv_obj_t *_label;

static void update_display(app_state_t state, const settings_t *s, uint8_t volume) {
    char buf[32];
    switch (state) {
        case STATE_VOLUME:
            if (volume >= TDA7439_VOL_MAX)
                snprintf(buf, sizeof(buf), "Volume\nMUTE");
            else
                snprintf(buf, sizeof(buf), "Volume\n%d dB", -(int)volume);
            break;
        case STATE_INPUT:
            snprintf(buf, sizeof(buf), "Input\n%d", s->input);
            break;
        case STATE_BASS:
            snprintf(buf, sizeof(buf), "Bass\n%d dB", (s->bass - 7) * 2);
            break;
        case STATE_MID:
            snprintf(buf, sizeof(buf), "Mid\n%d dB", (s->mid - 7) * 2);
            break;
        case STATE_TREBLE:
            snprintf(buf, sizeof(buf), "Treble\n%d dB", (s->treble - 7) * 2);
            break;
        case STATE_BALANCE:
            snprintf(buf, sizeof(buf), "Balance\n%d", s->balance);
            break;
        default:
            break;
    }
    lv_label_set_text(_label, buf);
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
    put_pixel(colour_grb(0, 32, 0));   // Red = off

    // Hardware init
    relay_init();
    gc9a01_init();
    gc9a01_lvgl_init();
    encoder_init(ENC_A, ENC_B, ENC_PB);

    // LVGL tick
    static repeating_timer_t lvgl_tick_timer;
    add_repeating_timer_ms(1, lvgl_tick_cb, NULL, &lvgl_tick_timer);

    // Placeholder display label
    _label = lv_label_create(lv_scr_act());
    lv_label_set_text(_label, "OFF");
    lv_obj_set_style_text_align(_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_label, LV_ALIGN_CENTER, 0, 0);

    // Load settings from flash
    settings_t settings;
    settings_load(&settings);

    // Volume lives in RAM only - always starts at 48 (mute) on cold boot
    uint8_t cur_volume = 48;

    app_state_t state = STATE_OFF;
    uint32_t last_activity_ms = 0;

    while (true) {
        int8_t  delta     = encoder_get_delta();
        bool    pressed   = encoder_button_pressed();
        bool    held      = encoder_button_held();

        if (pressed || held) {
            last_activity_ms = to_ms_since_boot(get_absolute_time());
        }
        
        switch (state) {

            // -------------------------------------------
            case STATE_OFF:
                if (pressed) {
                    state = STATE_POWERING_ON;
                    printf("Powering on...\n");
                }
                break;

            // -------------------------------------------
            case STATE_POWERING_ON:
                relay_power_on();
                sleep_ms(1000);
                put_pixel(colour_grb(32, 0, 0));   // Green = on
                if (tda7439_init()) {
                    apply_settings(&settings, cur_volume);
                    state = STATE_VOLUME;
                    update_display(state, &settings, cur_volume);
                    printf("Power on OK\n");
                } else {
                    // TDA7439 failed - power back off
                    relay_power_off();
                    put_pixel(colour_grb(0, 32, 0));   // Red = off
                    state = STATE_OFF;
                    lv_label_set_text(_label, "TDA ERROR");
                    printf("TDA7439 init failed\n");
                }
                break;

            // -------------------------------------------
            case STATE_VOLUME:
                if (delta != 0) {
                    int16_t v = (int16_t)cur_volume - delta;  // Note: minus delta
                    if (v < TDA7439_VOL_MIN) v = TDA7439_VOL_MIN;
                    if (v > TDA7439_VOL_MAX) v = TDA7439_VOL_MAX;
                    cur_volume = (uint8_t)v;
                    tda7439_set_volume(cur_volume);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    state = STATE_INPUT;
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_INPUT:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.input + delta;
                    if (v < TDA7439_INPUT_MIN) v = TDA7439_INPUT_MIN;
                    if (v > TDA7439_INPUT_MAX) v = TDA7439_INPUT_MAX;
                    settings.input = (uint8_t)v;
                    tda7439_set_input(settings.input);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    state = STATE_BASS;
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_BASS:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.bass + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.bass = (uint8_t)v;
                    tda7439_set_bass(settings.bass);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    state = STATE_MID;
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_MID:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.mid + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.mid = (uint8_t)v;
                    tda7439_set_mid(settings.mid);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    state = STATE_TREBLE;
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_TREBLE:
                if (delta != 0) {
                    int8_t v = (int8_t)settings.treble + delta;
                    if (v < TDA7439_TONE_MIN) v = TDA7439_TONE_MIN;
                    if (v > TDA7439_TONE_MAX) v = TDA7439_TONE_MAX;
                    settings.treble = (uint8_t)v;
                    tda7439_set_treble(settings.treble);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    state = STATE_BALANCE;
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_BALANCE:
                if (delta != 0) {
                    int8_t v = settings.balance + delta;
                    if (v < TDA7439_BAL_MIN) v = TDA7439_BAL_MIN;
                    if (v > TDA7439_BAL_MAX) v = TDA7439_BAL_MAX;
                    settings.balance = v;
                    tda7439_set_balance(settings.balance);
                    update_display(state, &settings, cur_volume);
                }
                if (pressed) {
                    settings_save(&settings);   // Save on cycling back to volume
                    printf("Power recovery - settings saved\n");
                    state = STATE_VOLUME;   // Loop back to volume
                    update_display(state, &settings, cur_volume);
                }
                if (held) state = STATE_POWERING_OFF;
                break;

            // -------------------------------------------
            case STATE_POWERING_OFF:
                tda7439_mute();
                relay_power_off();
                put_pixel(colour_grb(0, 32, 0));   // Red = off
                lv_label_set_text(_label, "OFF");
                state = STATE_OFF;
                printf("Power off - settings saved\n");
                break;
        }

        lv_timer_handler();
        sleep_ms(5);

        // Track last encoder activity
        if (delta != 0) {
            last_activity_ms = to_ms_since_boot(get_absolute_time());
        }

        // Auto-return to volume after 5 seconds of inactivity
        if (state > STATE_POWERING_ON && 
            state != STATE_VOLUME && 
            state != STATE_POWERING_OFF) {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if ((now - last_activity_ms) > 5000) {
                state = STATE_VOLUME;
                update_display(state, &settings, cur_volume);
                printf("Timeout - returning to volume\n");
                last_activity_ms = now;  // Prevent immediate re-trigger
            }
        }
    }
}