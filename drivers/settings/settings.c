#include "settings.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

// Use the last 4KB sector of flash
// RP2040-Zero has 2MB flash = 0x200000 bytes
#define FLASH_SIZE_BYTES    (2 * 1024 * 1024)
#define SETTINGS_OFFSET     (FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC      0xDEADBEEF

// XIP base is where flash appears in the address space
#define SETTINGS_ADDR       (XIP_BASE + SETTINGS_OFFSET)

void settings_load(settings_t *s) {
    const settings_t *stored = (const settings_t *)SETTINGS_ADDR;

    if (stored->magic == SETTINGS_MAGIC) {
        // Valid settings found in flash
        memcpy(s, stored, sizeof(settings_t));
        printf("Settings loaded from flash\n");
    } else {
        // Flash not written yet - use defaults
        s->input   = SETTINGS_DEFAULT_INPUT;
        s->bass    = SETTINGS_DEFAULT_BASS;
        s->mid     = SETTINGS_DEFAULT_MID;
        s->treble  = SETTINGS_DEFAULT_TREBLE;
        s->balance = SETTINGS_DEFAULT_BALANCE;
        s->magic   = SETTINGS_MAGIC;
        printf("No settings in flash - using defaults\n");
    }
}

void settings_save(const settings_t *s) {
    // Prepare page-aligned buffer (flash writes must be 256 bytes)
    uint8_t buf[FLASH_PAGE_SIZE];
    memset(buf, 0xFF, sizeof(buf));
    memcpy(buf, s, sizeof(settings_t));

    // Flash operations must be done with interrupts disabled
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(SETTINGS_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_OFFSET, buf, FLASH_PAGE_SIZE);
    restore_interrupts(irq);

    printf("Settings saved to flash\n");
}