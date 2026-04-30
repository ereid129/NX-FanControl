# NX-FanControl

A Nintendo Switch fan control overlay and sysmodule. Fork of [ppkantorski/NX-FanControl](https://github.com/ppkantorski/NX-FanControl) with added monitoring and settings features.

---

## Features

### Fan Control
- Set a custom 5-point fan curve (temperature → fan speed %)
- Fine-tune slider values by ±1 using **R + D-pad left/right** (default step is ±5)
- Fan curve is applied by the background sysmodule every 100ms
- Hysteresis control: fan turns on at P1 temp, turns off at P0 temp — eliminates clicking from threshold oscillation
- Hardware minimum enforced: fan runs at 20% minimum or fully off, never between

### Monitoring
- Real-time fan speed display
- SoC, PCB, and Skin temperature display with gradient color
- Temperature rows are individually toggleable in Settings

### Settings Menu (D-pad right to open, D-pad left to close)
- Toggle SoC / PCB / Skin temperature display
- **Fan curve presets:** Silent, Balanced, Performance
- **Save as Custom:** saves your current fan curve as a custom preset
- **Custom:** loads your saved custom curve
- **Game Profiles:** assign a preset per game — auto-applies when the game launches

### Default Temperature Display
On first launch, only SoC Temp is enabled. PCB and Skin can be enabled in Settings.

---

## Fan Curve Presets

Presets use a hysteresis design: **P0 is the turn-off temperature** (fan level 0%), **P1 is the turn-on temperature** (fan level ≥ 20%). The fan will not start until P1 is reached, and will not stop until temp drops back to P0. This prevents clicking from rapid on/off cycling near the threshold.

The minimum running speed is 20% — the hardware cannot sustain lower speeds. Fan level values between 1–19% are automatically snapped to 20% when editing a custom curve.

| Preset | P0 (off) | P1 (on) | P2 | P3 | P4 |
|---|---|---|---|---|---|
| Silent | 45°C \| 0% | 48°C \| 20% | 60°C \| 40% | 70°C \| 60% | 80°C \| 85% |
| Balanced | 37°C \| 0% | 40°C \| 20% | 57°C \| 55% | 67°C \| 80% | 80°C \| 100% |
| Performance | 35°C \| 0% | 38°C \| 20% | 50°C \| 60% | 60°C \| 85% | 70°C \| 100% |

---

## Installation

Copy the contents of `out/` to your SD card root.

```
out/
├── atmosphere/contents/00FF0000B378D640/
│   ├── exefs.nsp        ← sysmodule
│   ├── toolbox.json
│   └── flags/boot2.flag ← autostart on boot
└── switch/.overlays/
    └── NX-FanControl.ovl ← overlay
```

Open the Ultrahand overlay menu and enable NX-FanControl.

---

## Building from Source

### Requirements

[devkitPro](https://devkitpro.org/wiki/Getting_Started) with libnx

```bash
git clone --recurse-submodules https://github.com/SamuraiBuyer/NX-FanControl.git
cd NX-FanControl

# Build everything and stage to ./out/
make all

# Or build individually
make sysmodule
make overlay
```

---

## Save Files

All settings are stored on the SD card at `sdmc:/config/NX-FanControl/`:

| File | Contents |
|---|---|
| `config.dat` | Fan curve (5 temperature points) |
| `ui_settings.dat` | Temperature display toggles |
| `custom_curve.dat` | Saved custom fan curve |
| `game_profiles.dat` | Per-game preset assignments |

Delete any of these files to reset that setting to defaults.

---

## Credits

Based on [ppkantorski/NX-FanControl](https://github.com/ppkantorski/NX-FanControl). Uses [libultrahand](https://github.com/ppkantorski/libultrahand) for the overlay framework.
