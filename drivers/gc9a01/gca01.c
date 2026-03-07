#include "gc9a01.h"
#include <string.h>

// --- Low level helpers ---

static inline void cs_select(void) {
    gpio_put(GC9A01_PIN_CS, 0);
}

static inline void cs_deselect(void) {
    gpio_put(GC9A01_PIN_CS, 1);
}

static inline void set_data_mode(void) {
    gpio_put(GC9A01_PIN_DC, 1);
}

static inline void set_command_mode(void) {
    gpio_put(GC9A01_PIN_DC, 0);
}

static void write_command(uint8_t cmd) {
    set_command_mode();
    cs_select();
    spi_write_blocking(GC9A01_SPI_PORT, &cmd, 1);
    cs_deselect();
}

static void write_data(const uint8_t *data, size_t len) {
    set_data_mode();
    cs_select();
    spi_write_blocking(GC9A01_SPI_PORT, data, len);
    cs_deselect();
}

static void write_data_byte(uint8_t byte) {
    write_data(&byte, 1);
}

// --- Init sequence ---

void gc9a01_init(void) {
    // SPI init at 62.5MHz (max the GC9A01 supports is 100MHz, so this is safe)
    spi_init(GC9A01_SPI_PORT, 62500000);
    spi_set_format(GC9A01_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(GC9A01_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(GC9A01_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(GC9A01_PIN_CS);
    gpio_set_dir(GC9A01_PIN_CS, GPIO_OUT);
    gpio_put(GC9A01_PIN_CS, 1);

    gpio_init(GC9A01_PIN_DC);
    gpio_set_dir(GC9A01_PIN_DC, GPIO_OUT);

    gpio_init(GC9A01_PIN_RST);
    gpio_set_dir(GC9A01_PIN_RST, GPIO_OUT);

    // Hardware reset
    gpio_put(GC9A01_PIN_RST, 1);
    sleep_ms(10);
    gpio_put(GC9A01_PIN_RST, 0);
    sleep_ms(10);
    gpio_put(GC9A01_PIN_RST, 1);
    sleep_ms(120);

    // Init sequence
    write_command(0xEF);

    write_command(0xEB);
    write_data_byte(0x14);

    write_command(0xFE);
    write_command(0xEF);

    write_command(0xEB);
    write_data_byte(0x14);

    write_command(0x84);
    write_data_byte(0x40);

    write_command(0x85);
    write_data_byte(0xFF);

    write_command(0x86);
    write_data_byte(0xFF);

    write_command(0x87);
    write_data_byte(0xFF);

    write_command(0x88);
    write_data_byte(0x0A);

    write_command(0x89);
    write_data_byte(0x21);

    write_command(0x8A);
    write_data_byte(0x00);

    write_command(0x8B);
    write_data_byte(0x80);

    write_command(0x8C);
    write_data_byte(0x01);

    write_command(0x8D);
    write_data_byte(0xFF);

    write_command(0x8E);
    write_data_byte(0xFF);

    write_command(0x8F);
    write_data_byte(0xFF);

    write_command(0xC3);
    write_data_byte(0x1F);

    write_command(0xC4);
    write_data_byte(0x1F);

    write_command(0xC9);
    write_data_byte(0x25);

    write_command(0xBE);
    write_data_byte(0x11);

    write_command(0xE1);
    write_data_byte(0x10);
    write_data_byte(0x0E);

    write_command(0xDF);
    write_data_byte(0x21);
    write_data_byte(0x0C);
    write_data_byte(0x02);

    write_command(0xF0);    // Gamma 1
    write_data_byte(0x45);
    write_data_byte(0x09);
    write_data_byte(0x08);
    write_data_byte(0x08);
    write_data_byte(0x26);
    write_data_byte(0x2A);

    write_command(0xF1);    // Gamma 2
    write_data_byte(0x43);
    write_data_byte(0x70);
    write_data_byte(0x72);
    write_data_byte(0x36);
    write_data_byte(0x37);
    write_data_byte(0x6F);

    write_command(0xF2);    // Gamma 3
    write_data_byte(0x45);
    write_data_byte(0x09);
    write_data_byte(0x08);
    write_data_byte(0x08);
    write_data_byte(0x26);
    write_data_byte(0x2A);

    write_command(0xF3);    // Gamma 4
    write_data_byte(0x43);
    write_data_byte(0x70);
    write_data_byte(0x72);
    write_data_byte(0x36);
    write_data_byte(0x37);
    write_data_byte(0x6F);

    write_command(0xED);
    write_data_byte(0x1B);
    write_data_byte(0x0B);

    write_command(0xAE);
    write_data_byte(0x77);

    write_command(0xCD);
    write_data_byte(0x63);

    write_command(0x70);
    write_data_byte(0x07);
    write_data_byte(0x07);
    write_data_byte(0x04);
    write_data_byte(0x0E);
    write_data_byte(0x0F);
    write_data_byte(0x09);
    write_data_byte(0x07);
    write_data_byte(0x08);
    write_data_byte(0x03);

    write_command(0xE8);
    write_data_byte(0x34);

    write_command(0x62);
    write_data_byte(0x18);
    write_data_byte(0x0D);
    write_data_byte(0x71);
    write_data_byte(0xED);
    write_data_byte(0x70);
    write_data_byte(0x70);
    write_data_byte(0x18);
    write_data_byte(0x0F);
    write_data_byte(0x71);
    write_data_byte(0xEF);
    write_data_byte(0x70);
    write_data_byte(0x70);

    write_command(0x63);
    write_data_byte(0x18);
    write_data_byte(0x11);
    write_data_byte(0x71);
    write_data_byte(0xF1);
    write_data_byte(0x70);
    write_data_byte(0x70);
    write_data_byte(0x18);
    write_data_byte(0x13);
    write_data_byte(0x71);
    write_data_byte(0xF3);
    write_data_byte(0x70);
    write_data_byte(0x70);

    write_command(0x64);
    write_data_byte(0x28);
    write_data_byte(0x29);
    write_data_byte(0xF1);
    write_data_byte(0x01);
    write_data_byte(0xF1);
    write_data_byte(0x00);
    write_data_byte(0x07);

    write_command(0x66);
    write_data_byte(0x3C);
    write_data_byte(0x00);
    write_data_byte(0xCD);
    write_data_byte(0x67);
    write_data_byte(0x45);
    write_data_byte(0x45);
    write_data_byte(0x10);
    write_data_byte(0x00);
    write_data_byte(0x00);
    write_data_byte(0x00);

    write_command(0x67);
    write_data_byte(0x00);
    write_data_byte(0x3C);
    write_data_byte(0x00);
    write_data_byte(0x00);
    write_data_byte(0x00);
    write_data_byte(0x01);
    write_data_byte(0x54);
    write_data_byte(0x10);
    write_data_byte(0x32);
    write_data_byte(0x98);

    write_command(0x74);
    write_data_byte(0x10);
    write_data_byte(0x85);
    write_data_byte(0x80);
    write_data_byte(0x00);
    write_data_byte(0x00);
    write_data_byte(0x4E);
    write_data_byte(0x00);

    write_command(0x98);
    write_data_byte(0x3E);
    write_data_byte(0x07);

    write_command(0x35);    // Tearing effect on
    write_command(0x21);    // Display inversion on

    write_command(0x11);    // Sleep out
    sleep_ms(120);

    write_command(0x29);    // Display on
    sleep_ms(20);

    // Memory access control - set pixel format
    write_command(0x36);
    write_data_byte(0x48);  // MX, BGR

    write_command(0x3A);    // Pixel format
    write_data_byte(0x05);  // 16 bit RGB565
}

// --- Window and pixel write ---

void gc9a01_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    write_command(0x2A);    // Column address set
    uint8_t col_data[] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF)
    };
    write_data(col_data, 4);

    write_command(0x2B);    // Row address set
    uint8_t row_data[] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF)
    };
    write_data(row_data, 4);

    write_command(0x2C);    // Memory write
}

void gc9a01_write_pixels(uint16_t *pixels, uint32_t count) {
    set_data_mode();
    cs_select();
    spi_write_blocking(GC9A01_SPI_PORT, (uint8_t *)pixels, count * 2);
    cs_deselect();
}