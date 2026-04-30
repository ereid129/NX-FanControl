#pragma once
#include <tesla.hpp>
#include <switch.h>
#include <stdio.h>
#include <fancontrol.h>
#include "pwm.h"

#define SysFanControlID      0x00FF0000B378D640
#define SysFanControlB2FPath "/atmosphere/contents/00FF0000B378D640/flags/boot2.flag"

#define CUSTOM_CURVE_PATH   "./config/NX-FanControl/custom_curve.dat"
#define GAME_PROFILES_PATH  "./config/NX-FanControl/game_profiles.dat"
#define MAX_GAME_PROFILES   64

#define FAN_MIN_PCT     20  // hardware cannot sustain speeds below this
#define NUM_TEMP_POINTS  6  // number of points in the fan curve

// ── Custom curve ────────────────────────────────────────────────────────────

void SaveCustomCurve(TemperaturePoint* handheld, TemperaturePoint* docked);
bool LoadCustomCurve(TemperaturePoint* handheld, TemperaturePoint* docked);

// ── Presets ─────────────────────────────────────────────────────────────────

struct FanPreset {
    TemperaturePoint handheld[NUM_TEMP_POINTS];
    TemperaturePoint docked[NUM_TEMP_POINTS];
};

enum PresetIndex {
    PRESET_IDX_NONE        = 0,
    PRESET_IDX_SILENT      = 1,
    PRESET_IDX_BALANCED    = 2,
    PRESET_IDX_PERFORMANCE = 3,
    PRESET_IDX_CUSTOM      = 4,
};

extern const FanPreset PRESET_SILENT;
extern const FanPreset PRESET_BALANCED;
extern const FanPreset PRESET_PERFORMANCE;

const char*      PresetIndexName(int idx);
const FanPreset* PresetIndexTable(int idx);  // nullptr for NONE / CUSTOM
int  DetectCurvePreset(TemperaturePoint* handheld, TemperaturePoint* docked);
void ApplyPresetData(TemperaturePoint* handheld, TemperaturePoint* docked, bool* changed,
                     const TemperaturePoint* presetH, const TemperaturePoint* presetD);

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
void  RestartSysmodule();

// ── Hardware reads ───────────────────────────────────────────────────────────

void  WriteFanConfig(TemperaturePoint* handheld, TemperaturePoint* docked);
void  ReadFanConfig(TemperaturePoint** handheld, TemperaturePoint** docked);

int   ReadFanPercent();
float ReadSocTemp();
float ReadPcbTemp();
float ReadSkinTemp();

// ── Shared monitoring ────────────────────────────────────────────────────────

struct MonitoringState {
    int   fanPct      = -1;
    float socTemp     = -1.0f;
    float pcbTemp     = -1.0f;
    float skinTemp    = -1.0f;
    std::string profileStr     = "Handheld";
    std::string curvePresetStr = "None";
    char  appIdBuf[17]    = {};
    char  fanBuf[8]       = {};
    char  socTempBuf[12]  = {};
    char  pcbTempBuf[12]  = {};
    char  skinTempBuf[12] = {};
    u64   lastTick = 0;
};

extern MonitoringState g_monitor;
bool UpdateMonitoring();   // returns true if a read occurred (500ms gate)
void DrawMonitoringHeader(tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h,
                          const std::string& hintStr, const std::string& hintIcon);
