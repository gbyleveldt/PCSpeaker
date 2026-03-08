#include "tda7439.h"
#include <stdio.h>

// Sub-addresses
#define REG_INPUT       0x00
#define REG_INPUT_GAIN  0x01
#define REG_VOLUME      0x02
#define REG_BASS        0x03
#define REG_MID         0x04
#define REG_TREBLE      0x05
#define REG_SPKR_R      0x06
#define REG_SPKR_L      0x07

// Tone register encoding lookup table
// Index 0-14 maps to -14dB to +14dB in 2dB steps
// Register values are non-linear above 0dB - see datasheet Table 11-13
static const uint8_t tone_reg[15] = {
    0x00,   // -14dB
    0x01,   // -12dB
    0x02,   // -10dB
    0x03,   //  -8dB
    0x04,   //  -6dB
    0x05,   //  -4dB
    0x06,   //  -2dB
    0x07,   //   0dB (centre)
    0x0E,   //  +2dB
    0x0D,   //  +4dB
    0x0C,   //  +6dB
    0x0B,   //  +8dB
    0x0A,   // +10dB
    0x09,   // +12dB
    0x08,   // +14dB
};

static void write_reg(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    int result = i2c_write_blocking(TDA7439_I2C_PORT, TDA7439_I2C_ADDR, buf, 2, false);
    if (result == PICO_ERROR_GENERIC) {
        printf("TDA7439: I2C write failed reg=0x%02X\n", reg);
    }
}

bool tda7439_init(void) {
    i2c_init(TDA7439_I2C_PORT, 100000);  // 100kHz

    gpio_set_function(TDA7439_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TDA7439_I2C_SCL, GPIO_FUNC_I2C);

    // Hardware pull-ups on board, no internal pull-ups needed

    // Probe the device
    uint8_t buf = 0;
    int result = i2c_write_blocking(TDA7439_I2C_PORT, TDA7439_I2C_ADDR, &buf, 1, false);
    if (result == PICO_ERROR_GENERIC) {
        printf("TDA7439: device not found at 0x%02X\n", TDA7439_I2C_ADDR);
        return false;
    }

    printf("TDA7439: found at 0x%02X\n", TDA7439_I2C_ADDR);

    // Set input gain to 0dB
    write_reg(REG_INPUT_GAIN, 0x00);

    // Mute speaker attenuators on init
    tda7439_mute();

    return true;
}

void tda7439_set_volume(uint8_t vol) {
    if (vol > TDA7439_VOL_MAX) vol = TDA7439_VOL_MAX;
    if (vol == TDA7439_VOL_MAX)
        write_reg(REG_VOLUME, 0x38);  // Mute
    else
        write_reg(REG_VOLUME, vol);
}

void tda7439_set_input(uint8_t input) {
    if (input < TDA7439_INPUT_MIN) input = TDA7439_INPUT_MIN;
    if (input > TDA7439_INPUT_MAX) input = TDA7439_INPUT_MAX;
    // Datasheet: IN1=0x03, IN2=0x02
    uint8_t reg_val = (input == 1) ? 0x03 : 0x02;
    write_reg(REG_INPUT, reg_val);
}

void tda7439_set_bass(uint8_t bass) {
    if (bass > TDA7439_TONE_MAX) bass = TDA7439_TONE_MAX;
    write_reg(REG_BASS, tone_reg[bass]);
}

void tda7439_set_mid(uint8_t mid) {
    if (mid > TDA7439_TONE_MAX) mid = TDA7439_TONE_MAX;
    write_reg(REG_MID, tone_reg[mid]);
}

void tda7439_set_treble(uint8_t treble) {
    if (treble > TDA7439_TONE_MAX) treble = TDA7439_TONE_MAX;
    write_reg(REG_TREBLE, tone_reg[treble]);
}

void tda7439_set_balance(int8_t balance) {
    if (balance < TDA7439_BAL_MIN) balance = TDA7439_BAL_MIN;
    if (balance > TDA7439_BAL_MAX) balance = TDA7439_BAL_MAX;

    uint8_t att_l = 0;
    uint8_t att_r = 0;

    if (balance < 0)
        att_r = (uint8_t)(-balance);   // Attenuate right for left balance
    else if (balance > 0)
        att_l = (uint8_t)(balance);    // Attenuate left for right balance

    write_reg(REG_SPKR_R, att_r);
    write_reg(REG_SPKR_L, att_l);
}

void tda7439_mute(void) {
    // Max attenuation on both speakers
    write_reg(REG_SPKR_R, 0x3F);
    write_reg(REG_SPKR_L, 0x3F);
}