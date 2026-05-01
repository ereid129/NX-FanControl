# NX-FanControl

A Nintendo Switch fan control overlay and sysmodule. Fork of [ppkantorski/NX-FanControl](https://github.com/ppkantorski/NX-FanControl) with added monitoring, dual-curve presets, and settings features.

---

## Features

### Fan Control
- Set a custom **6-point fan curve** (temperature → fan speed %)
- **Dual curves:** separate profiles for handheld and docked — applied automatically based on current mode
- Fine-tune slider values by ±1 using **R + D-pad left/right** (default step is ±5)
- Fan curve is applied by the background sysmodule every 100ms
- Hysteresis control: fan turns on at P1 temp, turns off at P0 temp — eliminates clicking from threshold oscillation
- Hardware minimum enforced: fan runs at 20% minimum or fully off, never between 1–19%
- Handheld fan hardware cap: 60%. Docked can reach 100%

### Monitoring
- Real-time fan speed display
- SoC, PCB, and Skin temperature display with gradient color
- Dock/handheld mode indicator
- Active fan curve profile label: **Default**, **Silent**, **Balanced**, **Performance**, **Custom**, or **Not Saved**
- Monitoring header visible across all menus (main, edit, settings)

### Settings Menu (D-pad right to open, D-pad left to close)
- Toggle SoC / PCB / Skin temperature display
- **Fan curve presets:** Silent, Balanced, Performance — each has separate handheld and docked curves
- **Save as Custom:** saves your current fan curve as a custom preset
- **Custom:** loads your saved custom curve
- **Game Profiles:** assign a preset per game — auto-applies when the game launches

### Default Temperature Display
On first launch, only SoC Temp is enabled. PCB and Skin can be enabled in Settings.

---

## Fan Curve Presets

Presets use a hysteresis design: **P0 is the turn-off temperature** (fan level 0%), **P1 is the turn-on temperature** (fan level ≥ 20%). The fan will not start until P1 is reached, and will not stop until temp drops back to P0. This prevents clicking from rapid on/off cycling near the threshold.

The minimum running speed is 20% — the hardware cannot sustain lower speeds.

### Handheld (60% max)

| Preset | P0 | P1 | P2 | P3 | P4 | P5 |
|---|---|---|---|---|---|---|
| Silent | 40°C \| 0% | 43°C \| 20% | 50°C \| 20% | 58°C \| 30% | 65°C \| 40% | 73°C \| 60% |
| Balanced | 37°C \| 0% | 40°C \| 20% | 48°C \| 20% | 55°C \| 40% | 60°C \| 60% | 60°C \| 60% |
| Performance | 33°C \| 0% | 36°C \| 20% | 44°C \| 20% | 50°C \| 40% | 56°C \| 60% | 60°C \| 60% |

### Docked (100% max)

| Preset | P0 | P1 | P2 | P3 | P4 | P5 |
|---|---|---|---|---|---|---|
| Silent | 40°C \| 0% | 43°C \| 20% | 50°C \| 20% | 58°C \| 25% | 67°C \| 50% | 77°C \| 80% |
| Balanced | 37°C \| 0% | 40°C \| 20% | 47°C \| 30% | 57°C \| 40% | 65°C \| 60% | 76°C \| 100% |
| Performance | 33°C \| 0% | 36°C \| 20% | 43°C \| 30% | 52°C \| 50% | 60°C \| 75% | 70°C \| 100% |

**Balanced** is closest to stock behavior. **Performance** is Balanced shifted ~4°C earlier. **Silent** shifts curves later with a gentler ramp and caps docked at 80%.

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
git clone --recurse-submodules https://github.com/ereid129/NX-FanControl.git
cd NX-FanControl

# Build everything and stage to ./out/
make all

# Or build individually (lib must be built first)
make lib/libfancontrol
make sysmodule
make overlay
```

---

## Save Files

All settings are stored on the SD card at `sdmc:/config/NX-FanControl/`:

| File | Contents |
|---|---|
| `config.dat` | Fan curve — 6 handheld points + 6 docked points |
| `ui_settings.dat` | Temperature display toggles |
| `custom_curve.dat` | Saved custom fan curve |
| `game_profiles.dat` | Per-game preset assignments |

Delete any of these files to reset that setting to defaults.

---

## Credits

Based on [ppkantorski/NX-FanControl](https://github.com/ppkantorski/NX-FanControl). Uses [libultrahand](https://github.com/ppkantorski/libultrahand) for the overlay framework.
