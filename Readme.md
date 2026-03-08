# PCSpeaker

A RP2040-based audio controller with LVGL UI, built as a drop-in replacement for an Arduino Nano controller driving a TDA7439 audio processor in a PC subwoofer amplifier.

## Hardware

| Component | Details |
|---|---|
| Controller | WaveShare RP2040-Zero |
| Display | GC9A01 240×240 round LCD (SPI) |
| Audio Processor | TDA7439DS volume/tone/input controller (I2C) |
| Input | Rotary encoder with push button |
| Status LED | WS2812 (onboard GP16) |
| Outputs | 2× relay — amp power (GP14), mute (GP15) |

## Pin Mapping

| Function | GPIO |
|---|---|
| GC9A01 SCK | GP10 |
| GC9A01 MOSI | GP11 |
| GC9A01 CS | GP13 |
| GC9A01 DC | GP8 |
| GC9A01 RST | GP9 |
| TDA7439 SDA | GP6 |
| TDA7439 SCL | GP7 |
| Encoder A | GP2 |
| Encoder B | GP3 |
| Encoder PB | GP4 |
| Relay – Amp | GP14 |
| Relay – Mute | GP15 |
| WS2812 | GP16 |

## Project Structure

```
PCSpeaker/
├── PCSpeaker.cpp           # Main application + state machine
├── lv_conf.h               # LVGL configuration
├── CMakeLists.txt
├── lvgl/                   # git submodule — release/v8.3
└── drivers/
    ├── gc9a01/             # GC9A01 SPI display driver
    ├── ws2812/             # WS2812 PIO driver
    ├── encoder/            # Rotary encoder (quadrature state machine)
    ├── relay/              # Relay sequencing driver
    ├── tda7439/            # TDA7439 I2C audio processor driver
    ├── settings/           # Flash storage for persistent settings
    └── ui/                 # LVGL UI — arc display, labels
```

## Features

- **State machine UI** — cycles through Volume → Input → Bass → Mid → Treble → Balance
- **Short press** — advance to next parameter
- **Long press** — power off and save settings to flash
- **Auto-return** — returns to Volume after 5 seconds of inactivity
- **Persistent settings** — Input, Bass, Mid, Treble, Balance saved to flash on cycle completion. Volume persists in RAM between power cycles but resets to MUTE on cold boot
- **WS2812 status** — Red = off, Green = volume, Blue = input, Amber = EQ/balance
- **LVGL arc display** — unidirectional arc for volume, bidirectional arc centred at 12 o'clock for tone and balance

## Power Sequencing

**On:** relay GP14 (amp power) → 500ms delay → TDA7439 init → relay GP15 (unmute)

**Off:** relay GP15 (mute) → 500ms delay → relay GP14 (amp power off)

## ⚠️ Important: TDA7439 with RP2040

The TDA7439 has a minimum I2C HIGH logic level (VIH) of 3V. While the RP2040 operates at 3.3V, this margin is too tight for reliable communication in practice — the chip will not respond.

**A bidirectional I2C level shifter is required.** The recommended solution is two BSS138 MOSFETs, one per I2C line:

```
RP2040 3.3V ──[ 4.7kΩ ]──┬──────────────────┬──[ 4.7kΩ ]── 5V
                          │                  │
                     (3.3V side)        (5V side → TDA7439)
                       Source               Drain
                           └──── BSS138 ────┘
                                   │
                                 Gate
                                   │
                                3.3V
```

- **Source** → RP2040 GPIO (3.3V side)
- **Drain** → TDA7439 I2C pin (5V side)
- **Gate** → 3.3V
- 4.7kΩ pull-up on both sides

The RP2040 GPIO pins are **not 5V tolerant** (absolute maximum 3.8V), so do not apply 5V pull-ups directly to GPIO pins. The level shifter protects the RP2040 while giving the TDA7439 the logic levels it needs.

## Encoder Implementation

The encoder uses a quadrature state machine rather than single-edge interrupts. Both A and B trigger interrupts on both edges. A 4-entry lookup table validates every state transition, ignoring invalid transitions caused by contact bounce. Raw counts are divided by the encoder's pulses-per-detent (4 for most common encoders) with the remainder kept, so fast spins never lose counts. This approach is reliable across all encoder types without needing arbitrary debounce timers on the rotation.

## Building

Requires Raspberry Pi Pico SDK 2.x and ARM GCC toolchain. Easiest setup is via the [Raspberry Pi Pico VS Code extension](https://marketplace.visualstudio.com/items?itemName=raspberry-pi.raspberry-pi-pico).

Clone with submodules:

```bash
git clone --recurse-submodules https://github.com/gbyleveldt/PCSpeaker.git
```

Build:

```bash
cd PCSpeaker
mkdir build && cd build
cmake ..
make
```

Flash the resulting `PCSpeaker.uf2` to the RP2040-Zero by holding BOOT, pressing RESET, releasing RESET, then releasing BOOT. The board appears as a USB drive — copy the `.uf2` file to it.

## Dependencies

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.x
- [LVGL](https://github.com/lvgl/lvgl) v8.3 (included as git submodule)