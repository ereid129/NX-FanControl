#pragma once
#include <tesla.hpp>
#include <switch.h>
#include <stdio.h>
#include <fancontrol.h>
#include "pwm.h"

#define SysFanControlID      0x00FF0000B378D640
#define SysFanControlB2FPath "/atmosphere/contents/00FF0000B378D640/flags/boot2.flag"

#define UI_SETTINGS_PATH    "./config/NX-FanControl/ui_settings.dat"
#define CUSTOM_CURVE_PATH   "./config/NX-FanControl/custom_curve.dat"
#define GAME_PROFILES_PATH  "./config/NX-FanControl/game_profiles.dat"
#define MAX_GAME_PROFILES   64

// ── UI settings ────────────────────────────────────────────────────────────

struct UiSettings {
    int showSocTemp  = 1;
    int showPcbTemp  = 0;
    int showSkinTemp = 0;
};

UiSettings* LoadUiSettings();
void        SaveUiSettings(UiSettings* s);

// ── Custom curve ────────────────────────────────────────────────────────────

void SaveCustomCurve(TemperaturePoint* table);
bool LoadCustomCurve(TemperaturePoint* table);

// ── Presets ─────────────────────────────────────────────────────────────────

enum PresetIndex {
    PRESET_IDX_NONE        = 0,
    PRESET_IDX_SILENT      = 1,
    PRESET_IDX_BALANCED    = 2,
    PRESET_IDX_PERFORMANCE = 3,
    PRESET_IDX_CUSTOM      = 4,
};

extern const TemperaturePoint PRESET_SILENT[5];
extern const TemperaturePoint PRESET_BALANCED[5];
extern const TemperaturePoint PRESET_PERFORMANCE[5];

const char*             PresetIndexName(int idx);
const TemperaturePoint* PresetIndexTable(int idx);  // nullptr for NONE / CUSTOM
void ApplyPresetData(TemperaturePoint* table, bool* changed, const TemperaturePoint* preset);

// ── Game profiles ────────────────────────────────────────────────────────────

struct GameProfileEntry {
    u64 titleId;
    int preset;
};

u64  GetRunningTitleId();
int  GetGameProfile(u64 titleId);
void SetGameProfile(u64 titleId, int preset);
void RemoveGameProfile(u64 titleId);

// ── Fan control helpers ──────────────────────────────────────────────────────

u64   IsRunning();
void  CreateB2F();
void  RemoveB2F();

// ── Hardware reads ───────────────────────────────────────────────────────────

int   ReadFanPercent();
float ReadSocTemp();
float ReadPcbTemp();
float ReadSkinTemp();
