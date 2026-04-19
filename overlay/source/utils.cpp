#include "utils.hpp"
#include <cmath>
#include <cstring>

extern PwmChannelSession g_fanSession;
extern I2cSession        g_tmpSession;
extern Result g_pwmCheck;
extern Result g_i2cCheck;
extern Result g_tmpCheck;
extern Result g_tcCheck;

// ── UI settings persistence ──────────────────────────────────────────────────

UiSettings* LoadUiSettings()
{
    UiSettings* s = new UiSettings();
    FILE* f = fopen(UI_SETTINGS_PATH, "rb");
    if (f) {
        fread(s, sizeof(UiSettings), 1, f);
        fclose(f);
    }
    return s;
}

void SaveUiSettings(UiSettings* s)
{
    FILE* f = fopen(UI_SETTINGS_PATH, "wb");
    if (f) {
        fwrite(s, sizeof(UiSettings), 1, f);
        fclose(f);
    }
}

// ── Custom curve ─────────────────────────────────────────────────────────────

void SaveCustomCurve(TemperaturePoint* table)
{
    FILE* f = fopen(CUSTOM_CURVE_PATH, "wb");
    if (f) {
        fwrite(table, sizeof(TemperaturePoint) * 5, 1, f);
        fclose(f);
    }
}

bool LoadCustomCurve(TemperaturePoint* table)
{
    FILE* f = fopen(CUSTOM_CURVE_PATH, "rb");
    if (!f) return false;
    bool ok = fread(table, sizeof(TemperaturePoint) * 5, 1, f) == 1;
    fclose(f);
    return ok;
}

// ── Presets ──────────────────────────────────────────────────────────────────

const TemperaturePoint PRESET_SILENT[5] = {
    { 20,  0.05f },
    { 45,  0.30f },
    { 55,  0.45f },
    { 65,  0.60f },
    { 100, 0.80f },
};

const TemperaturePoint PRESET_BALANCED[5] = {
    { 20,  0.10f },
    { 40,  0.50f },
    { 50,  0.60f },
    { 60,  0.70f },
    { 100, 1.00f },
};

const TemperaturePoint PRESET_PERFORMANCE[5] = {
    { 20,  0.20f },
    { 35,  0.50f },
    { 45,  0.70f },
    { 55,  0.85f },
    { 70,  1.00f },
};

const char* PresetIndexName(int idx)
{
    switch (idx) {
        case PRESET_IDX_SILENT:      return "Silent";
        case PRESET_IDX_BALANCED:    return "Balanced";
        case PRESET_IDX_PERFORMANCE: return "Performance";
        case PRESET_IDX_CUSTOM:      return "Custom";
        default:                     return "None";
    }
}

const TemperaturePoint* PresetIndexTable(int idx)
{
    switch (idx) {
        case PRESET_IDX_SILENT:      return PRESET_SILENT;
        case PRESET_IDX_BALANCED:    return PRESET_BALANCED;
        case PRESET_IDX_PERFORMANCE: return PRESET_PERFORMANCE;
        default:                     return nullptr;
    }
}

void ApplyPresetData(TemperaturePoint* table, bool* changed, const TemperaturePoint* preset)
{
    memcpy(table, preset, sizeof(TemperaturePoint) * 5);
    WriteConfigFile(table);
    *changed = true;
    if (IsRunning() != 0) {
        pmshellTerminateProgram(SysFanControlID);
        const NcmProgramLocation loc{ .program_id = SysFanControlID, .storageID = NcmStorageId_None };
        u64 pid = 0;
        pmshellLaunchProgram(0, &loc, &pid);
    }
}

// ── Game profiles ─────────────────────────────────────────────────────────────

u64 GetRunningTitleId()
{
    u64 appPid = 0;
    if (R_FAILED(pmdmntGetApplicationProcessId(&appPid)) || appPid == 0) return 0;
    u64 titleId = 0;
    if (R_FAILED(pminfoGetProgramId(&titleId, appPid))) return 0;
    return titleId;
}

static void readGameProfiles(GameProfileEntry* entries)
{
    memset(entries, 0, sizeof(GameProfileEntry) * MAX_GAME_PROFILES);
    FILE* f = fopen(GAME_PROFILES_PATH, "rb");
    if (f) {
        fread(entries, sizeof(GameProfileEntry), MAX_GAME_PROFILES, f);
        fclose(f);
    }
}

static void writeGameProfiles(GameProfileEntry* entries)
{
    FILE* f = fopen(GAME_PROFILES_PATH, "wb");
    if (f) {
        fwrite(entries, sizeof(GameProfileEntry), MAX_GAME_PROFILES, f);
        fclose(f);
    }
}

int GetGameProfile(u64 titleId)
{
    if (titleId == 0) return PRESET_IDX_NONE;
    GameProfileEntry entries[MAX_GAME_PROFILES];
    readGameProfiles(entries);
    for (int i = 0; i < MAX_GAME_PROFILES; i++) {
        if (entries[i].titleId == titleId) return entries[i].preset;
    }
    return PRESET_IDX_NONE;
}

void SetGameProfile(u64 titleId, int preset)
{
    if (titleId == 0) return;
    GameProfileEntry entries[MAX_GAME_PROFILES];
    readGameProfiles(entries);
    for (int i = 0; i < MAX_GAME_PROFILES; i++) {
        if (entries[i].titleId == titleId) {
            entries[i].preset = preset;
            writeGameProfiles(entries);
            return;
        }
    }
    for (int i = 0; i < MAX_GAME_PROFILES; i++) {
        if (entries[i].titleId == 0) {
            entries[i].titleId = titleId;
            entries[i].preset  = preset;
            writeGameProfiles(entries);
            return;
        }
    }
}

void RemoveGameProfile(u64 titleId)
{
    if (titleId == 0) return;
    GameProfileEntry entries[MAX_GAME_PROFILES];
    readGameProfiles(entries);
    for (int i = 0; i < MAX_GAME_PROFILES; i++) {
        if (entries[i].titleId == titleId) {
            entries[i].titleId = 0;
            entries[i].preset  = PRESET_IDX_NONE;
            writeGameProfiles(entries);
            return;
        }
    }
}

// ── Fan control helpers ───────────────────────────────────────────────────────

u64 IsRunning()
{
    u64 pid = 0;
    pmdmntGetProcessId(&pid, SysFanControlID);
    return pid;
}

void RemoveB2F()
{
    remove(SysFanControlB2FPath);
}

void CreateB2F()
{
    FILE *f = fopen(SysFanControlB2FPath, "w");
    if (f) fclose(f);
}

// ── Fan speed display ─────────────────────────────────────────────────────────

int ReadFanPercent()
{
    if (R_FAILED(g_pwmCheck)) return -1;
    double duty = 0.0;
    if (R_FAILED(pwmChannelSessionGetDutyCycle(&g_fanSession, &duty))) return -1;
    int pct = (int)std::round(100.0 - duty);
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

// ── TMP451 temperature sensor ─────────────────────────────────────────────────

static constexpr u8 TMP451_SOC_INT_REG = 0x01;
static constexpr u8 TMP451_PCB_INT_REG = 0x00;
static constexpr u8 TMP451_SOC_DEC_REG = 0x10;
static constexpr u8 TMP451_PCB_DEC_REG = 0x15;

static Result tmp451Read(u8 reg, u8 *out)
{
    struct {
        u8 send;
        u8 sendLength;
        u8 sendData;
        u8 receive;
        u8 receiveLength;
    } cmdList = {
        .send          = 0 | (I2cTransactionOption_Start << 6),
        .sendLength    = 1,
        .sendData      = reg,
        .receive       = 1 | (I2cTransactionOption_All  << 6),
        .receiveLength = 1,
    };

    u8 val = 0;
    Result rc = i2csessionExecuteCommandList(&g_tmpSession, &val, sizeof(val),
                                            &cmdList, sizeof(cmdList));
    if (R_FAILED(rc)) return rc;
    *out = val;
    return 0;
}

float ReadSocTemp()
{
    if (R_FAILED(g_tmpCheck)) return -1.0f;
    u8 integer = 0, dec = 0;
    if (R_FAILED(tmp451Read(TMP451_SOC_INT_REG, &integer))) return -1.0f;
    tmp451Read(TMP451_SOC_DEC_REG, &dec);
    u8 decimals = ((u16)(dec >> 4) * 625) / 100;
    return (float)integer + (float)decimals / 100.0f;
}

float ReadPcbTemp()
{
    if (R_FAILED(g_tmpCheck)) return -1.0f;
    u8 integer = 0, dec = 0;
    if (R_FAILED(tmp451Read(TMP451_PCB_INT_REG, &integer))) return -1.0f;
    tmp451Read(TMP451_PCB_DEC_REG, &dec);
    u8 decimals = ((u16)(dec >> 4) * 625) / 100.0f;
    return (float)integer + (float)decimals / 100.0f;
}

float ReadSkinTemp()
{
    if (R_FAILED(g_tcCheck)) return -1.0f;
    s32 milliC = 0;
    if (R_FAILED(tcGetSkinTemperatureMilliC(&milliC))) return -1.0f;
    return (float)milliC / 1000.0f;
}
