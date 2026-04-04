# TODO

Deferred work items for PCSpeaker.

---

## Pending

- [ ] **Enclosure CAD files** — the enclosure was designed in Fusion 360 and 3D printed in ABS. The CAD files have not been published to the repo. Publish the Fusion 360 file or export as STEP for reference.

- [ ] **Display brightness control** — the GC9A01 backlight is currently at full brightness with no software control. The BL pin is wired but no PWM is configured. Adding LEDC/PWM backlight control would allow dimming after inactivity.

- [ ] **Input labelling** — inputs are labelled 1 and 2 in the UI. If the hardware configuration is known (e.g. CD, AUX), the labels could be made descriptive in `tda7439.h`.

---

*Last updated: April 2026*
