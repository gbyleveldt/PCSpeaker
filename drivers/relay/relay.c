#include "relay.h"

void relay_init(void) {
    gpio_init(RELAY_AMP);
    gpio_set_dir(RELAY_AMP, GPIO_OUT);
    gpio_put(RELAY_AMP, 0);

    gpio_init(RELAY_MUTE);
    gpio_set_dir(RELAY_MUTE, GPIO_OUT);
    gpio_put(RELAY_MUTE, 0);
}

void relay_power_on(void) {
    gpio_put(RELAY_AMP, 1);    // Power amps
    sleep_ms(500);             // Wait for amp to stabilise
    gpio_put(RELAY_MUTE, 1);   // Unmute
}

void relay_power_off(void) {
    gpio_put(RELAY_MUTE, 0);   // Mute first
    sleep_ms(500);             // Wait
    gpio_put(RELAY_AMP, 0);    // Power off amps
}