# Development Notes

This document captures architectural decisions, rationale, and project-specific context for maintainers. It is not a user guide — see README for build and usage instructions.

For reusable patterns and gotchas that apply beyond this project, see the skill files at `gbyleveldt/claude-skills`.

---

## TDA7439 Volume Convention: Inverted Scale

The TDA7439 volume register is inverted: **0 = maximum volume, 48 = mute.** This is the hardware convention for the chip, not a software choice.

`cur_volume` in `PCSpeaker.cpp` starts at 48 (mute) on cold boot. Encoder rotation subtracts the delta from `cur_volume` — turning clockwise (positive delta) lowers the register value, increasing the actual output level. This looks backwards in the code but is correct.

Do not refactor this to feel more intuitive without understanding the TDA7439 register map.

---

## Volume Is Not Persisted to Flash — By Design

Input, Bass, Mid, Treble, and Balance are saved to flash and restored on power-on. **Volume is not.** It always starts at mute (48) after a cold boot.

**Why:** A cold power cycle (unplugging the amp) implies the user is done listening. Restoring the previous volume automatically on next power-on risks unexpectedly loud audio if someone powers on without expecting it. In contrast, EQ settings are genuinely set-and-forget and don't carry the same risk.

Volume is preserved across the relay power sequence (short press power-on, long press power-off) within the same session — only a firmware restart or cold boot resets it.

---

## Settings Save Strategy: End-of-Cycle Only

Settings are written to flash **only** when the user completes a full parameter cycle: Balance → short press → returns to Volume. This is the only code path that calls `settings_save()`.

**Why:** Writing to flash on every encoder click would accumulate unnecessary write cycles. The TDA7439 updates in real time regardless — the flash write is only for persistence across power cycles. Completing the cycle is a natural “done adjusting” signal.

**Implication:** If the user powers off mid-cycle (e.g. adjusts Bass then long-presses), the new Bass value is applied to the TDA7439 for the current session but not saved. The previous value will restore on next boot.

---

## Power Sequencing: Why the Delays

Power-on sequence: `relay_power_on()` → 1000ms fade → `tda7439_init()` → `relay_power_off(mute)` (unmute).

The 1000ms WS2812 fade during power-on provides the amp power supply time to stabilise. The TDA7439 is powered from the amp supply rail, not the RP2040 3.3V. If `tda7439_init()` is called before the supply settles, the I2C transaction may fail or the chip may not respond reliably.

Power-off sequence: `tda7439_mute()` → `relay_power_off()` → 500ms fade. The mute command is sent before killing the relay to avoid audible pops from the amp switching off under signal.

---

## Post-Power-On Button Event Flush

At the end of `STATE_POWERING_ON`, after the amp comes online:

```cpp
encoder_button_held();      // Discard
encoder_button_pressed();   // Discard
```

This explicitly discards any queued button events before transitioning to `STATE_VOLUME`. Without this, if the user is still holding the power button when the boot sequence completes, the held event would immediately trigger `STATE_POWERING_OFF` — the amp would power on and then immediately power off again.

Do not remove these calls.

---

## on_state_enter() Pattern: Centralised UI Updates

`on_state_enter()` owns all display and LED updates when the state machine transitions. Individual state `case` blocks handle only encoder input and state transitions — they never call display or LED functions directly.

**Why:** Separating state logic from display logic keeps each case block small and readable, and ensures that display and LED state are always consistent with the machine state. If a new state is added, the display update goes in `on_state_enter()`, not scattered through the switch.

The same pattern is used in the auto-return timeout path and in the power-off sequence, both of which call `on_state_enter()` rather than performing display updates inline.

---

## Auto-Return Timer

From any EQ state (Input, Bass, Mid, Treble, Balance), 5 seconds of inactivity returns to `STATE_VOLUME`. The timer resets on any encoder activity — delta, short press, or hold.

The timer is intentionally not active in `STATE_OFF` or `STATE_VOLUME` (no point returning to Volume from Volume or from Off).

---

## Hardware Constraints: GP17–GP25 Unusable

Same constraint as all RP2040-Zero projects: GP17–GP25 are underside pads only, not available for through-hole use. All GPIOs are constrained to GP0–GP15 and GP26–GP29.

GP16 is the onboard WS2812 LED (`PICO_DEFAULT_WS2812_PIN`). It is not available for other use.

---

## Skill File References

The following gotchas apply to this project but are documented in the skill files — do not duplicate them here:

- **TDA7439 I2C level shifter (BSS138)** — see `components/tda7439.md`
- **Encoder quadrature state machine and button debounce fix** — see `components/encoder-driver.md`
- **GC9A01 display driver and colour swap** — see `components/gc9a01.md`
- **WS2812 GRB byte order and PIO pattern** — see `components/ws2812.md`
- **LVGL v8.3 tick source requirement and custom.cmake edit** — see `components/lvgl.md`

---

*Created April 2026.*
