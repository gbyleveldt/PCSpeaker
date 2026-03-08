#include "encoder.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

static uint _pin_a;
static uint _pin_b;
static uint _pin_pb;

static volatile int8_t   _delta         = 0;
static volatile bool     _pb_event      = false;
static volatile bool     _pb_state      = false;
static volatile bool     _pb_held       = false;
static volatile uint32_t _pb_press_time = 0;
static volatile uint32_t _last_pb_time  = 0;

#define DEBOUNCE_MS  150
#define HOLD_MS      1000

// Quadrature state machine table
// Index by previous state (2 bits) and current state (2 bits)
// +1 = CW, -1 = CCW, 0 = invalid/bounce
static const int8_t _qem_table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

static uint8_t _last_ab = 0;

static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (gpio == _pin_a || gpio == _pin_b) {
        uint8_t a = gpio_get(_pin_a) ? 1 : 0;
        uint8_t b = gpio_get(_pin_b) ? 1 : 0;
        uint8_t current_ab = (a << 1) | b;
        int8_t direction = _qem_table[(_last_ab << 2) | current_ab];
        if (direction != 0)
            _delta += direction;
        _last_ab = current_ab;
    }

    if (gpio == _pin_pb) {
        if ((now - _last_pb_time) > DEBOUNCE_MS) {
            _last_pb_time = now;
            if (events & GPIO_IRQ_EDGE_FALL) {
                _pb_state      = true;
                _pb_press_time = now;
                _pb_held       = false;
            } else if (events & GPIO_IRQ_EDGE_RISE) {
                _pb_state = false;
                // Only fire short press event on release if it wasn't a long press
                if (!_pb_held) {
                    _pb_event = true;
                }
            }
        }
    }
}

void encoder_init(uint pin_a, uint pin_b, uint pin_pb) {
    _pin_a  = pin_a;
    _pin_b  = pin_b;
    _pin_pb = pin_pb;

    gpio_init(pin_a);
    gpio_set_dir(pin_a, GPIO_IN);

    gpio_init(pin_b);
    gpio_set_dir(pin_b, GPIO_IN);

    gpio_init(pin_pb);
    gpio_set_dir(pin_pb, GPIO_IN);

    // Read initial state
    uint8_t a = gpio_get(_pin_a) ? 1 : 0;
    uint8_t b = gpio_get(_pin_b) ? 1 : 0;
    _last_ab = (a << 1) | b;

    // Interrupt on both edges of both A and B
    gpio_set_irq_enabled_with_callback(_pin_a,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_irq_handler);

    gpio_set_irq_enabled(_pin_b,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    gpio_set_irq_enabled(_pin_pb,
        GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
}

int8_t encoder_get_delta(void) {
    uint32_t irq_status = save_and_disable_interrupts();
    int8_t steps = 0;
    
    // Only report complete detents (4 pulses per detent)
    if (_delta >= 4) {
        steps = _delta / 4;
        _delta -= steps * 4;
    } else if (_delta <= -4) {
        steps = _delta / 4;
        _delta -= steps * 4;
    }
    
    restore_interrupts(irq_status);
    return steps;
}

bool encoder_button_pressed(void) {
    uint32_t irq_status = save_and_disable_interrupts();
    bool event = _pb_event;
    _pb_event = false;
    restore_interrupts(irq_status);
    return event;
}

bool encoder_button_held(void) {
    if (_pb_state && !_pb_held) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - _pb_press_time) > HOLD_MS) {
            _pb_held = true;
            return true;
        }
    }
    return false;
}