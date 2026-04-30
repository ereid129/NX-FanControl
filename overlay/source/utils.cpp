#include "utils.hpp"
#include <cmath>
#include <cstring>

extern PwmChannelSession g_fanSession;
extern I2cSession        g_tmpSession;
extern Result  g_pwmCheck;
extern Result  g_i2cCheck;
extern Result  g_tmpCheck;
extern Result  g_tcCheck;
extern Service g_apmRawSrv;
extern Result  g_apmRawCheck;

// ── Custom curve ─────────────────────────────────────────────────────────────

void WriteFanConfig(TemperaturePoint* handheld, TemperaturePoint* docked)
{
    FILE* f = fopen(CONFIG_FILE, "wb");
    if (f) {
        fwrite(handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fwrite(docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fclose(f);
    }
}

void ReadFanConfig(TemperaturePoint** handheld, TemperaturePoint** docked)
{
    static const TemperaturePoint defaultHandheld[NUM_TEMP_POINTS] = {
        { 37, 0.00f }, { 40, 0.20f }, { 48, 0.20f },
        { 55, 0.40f }, { 60, 0.60f }, { 60, 0.60f },
    };
    static const TemperaturePoint defaultDocked[NUM_TEMP_POINTS] = {
        { 37, 0.00f }, { 40, 0.20f }, { 53, 0.30f },
        { 57, 0.40f }, { 65, 0.60f }, { 76, 1.00f },
    };
    *handheld = (TemperaturePoint*)malloc(sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    *docked   = (TemperaturePoint*)malloc(sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    memcpy(*handheld, defaultHandheld, sizeof(defaultHandheld));
    memcpy(*docked,   defaultDocked,   sizeof(defaultDocked));

    FILE* f = fopen(CONFIG_FILE, "rb");
    if (f) {
        if (fread(*handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) != 1 ||
            fread(*docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) != 1) {
            memcpy(*handheld, defaultHandheld, sizeof(defaultHandheld));
            memcpy(*docked,   defaultDocked,   sizeof(defaultDocked));
        }
        fclose(f);
    }
}

void SaveCustomCurve(TemperaturePoint* handheld, TemperaturePoint* docked)
{
    FILE* f = fopen(CUSTOM_CURVE_PATH, "wb");
    if (f) {
        fwrite(handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fwrite(docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f);
        fclose(f);
    }
}

bool LoadCustomCurve(TemperaturePoint* handheld, TemperaturePoint* docked)
{
    FILE* f = fopen(CUSTOM_CURVE_PATH, "rb");
    if (!f) return false;
    bool ok = fread(handheld, sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) == 1 &&
              fread(docked,   sizeof(TemperaturePoint) * NUM_TEMP_POINTS, 1, f) == 1;
    fclose(f);
    return ok;
}

// ── Presets ──────────────────────────────────────────────────────────────────

// P0.fanLevel = 0.0 ensures fan is fully off below P0.temp.
// P1.fanLevel >= 0.20 (hardware minimum).
// Gap between P0.temp and P1.temp is the hysteresis band.
const FanPreset PRESET_SILENT = {
    // handheld (hardware cap 60%) — balanced curve shifted later, gentler ramp
    { { 40, 0.00f }, { 43, 0.20f }, { 50, 0.20f }, { 58, 0.30f }, { 65, 0.40f }, { 73, 0.60f } },
    // docked — same shift, gentler ramp, caps at 80%
    { { 40, 0.00f }, { 43, 0.20f }, { 50, 0.20f }, { 58, 0.25f }, { 67, 0.50f }, { 77, 0.80f } },
};

const FanPreset PRESET_BALANCED = {
    // handheld
    { { 37, 0.00f }, { 40, 0.20f }, { 48, 0.20f }, { 55, 0.40f }, { 60, 0.60f }, { 60, 0.60f } },
    // docked
    { { 37, 0.00f }, { 40, 0.20f }, { 47, 0.30f }, { 57, 0.40f }, { 65, 0.60f }, { 76, 1.00f } },
};

const FanPreset PRESET_PERFORMANCE = {
    // handheld (hardware cap 60%) — balanced curve shifted ~4C earlier
    { { 33, 0.00f }, { 36, 0.20f }, { 44, 0.20f }, { 50, 0.40f }, { 56, 0.60f }, { 60, 0.60f } },
    // docked — balanced curve shifted ~4C earlier, upper points pushed harder
    { { 33, 0.00f }, { 36, 0.20f }, { 43, 0.30f }, { 52, 0.50f }, { 60, 0.75f }, { 70, 1.00f } },
};

const char* PresetIndexName(int idx)
{
    switch (idx) {
        case PRESET_IDX_SILENT:      return "Silent";
        case PRESET_IDX_BALANCED:    return "Balanced";
        case PRESET_IDX_PERFORMANCE: return "Performance";
        case PRESET_IDX_CUSTOM:      return "Custom";
        default:                     return "Default";
    }
}

const FanPreset* PresetIndexTable(int idx)
{
    switch (idx) {
        case PRESET_IDX_SILENT:      return &PRESET_SILENT;
        case PRESET_IDX_BALANCED:    return &PRESET_BALANCED;
        case PRESET_IDX_PERFORMANCE: return &PRESET_PERFORMANCE;
        default:                     return nullptr;
    }
}

int DetectCurvePreset(TemperaturePoint* h, TemperaturePoint* d)
{
    const FanPreset* presets[] = { &PRESET_SILENT, &PRESET_BALANCED, &PRESET_PERFORMANCE };
    const int        indices[] = { PRESET_IDX_SILENT, PRESET_IDX_BALANCED, PRESET_IDX_PERFORMANCE };
    for (int p = 0; p < 3; p++) {
        bool match = true;
        for (int i = 0; i < NUM_TEMP_POINTS && match; i++) {
            if (h[i].temperature_c != presets[p]->handheld[i].temperature_c ||
                h[i].fanLevel_f    != presets[p]->handheld[i].fanLevel_f    ||
                d[i].temperature_c != presets[p]->docked[i].temperature_c   ||
                d[i].fanLevel_f    != presets[p]->docked[i].fanLevel_f)
                match = false;
        }
        if (match) return indices[p];
    }
    TemperaturePoint tmpH[NUM_TEMP_POINTS], tmpD[NUM_TEMP_POINTS];
    if (LoadCustomCurve(tmpH, tmpD)) {
        bool match = true;
        for (int i = 0; i < NUM_TEMP_POINTS && match; i++) {
            if (h[i].temperature_c != tmpH[i].temperature_c ||
                h[i].fanLevel_f    != tmpH[i].fanLevel_f    ||
                d[i].temperature_c != tmpD[i].temperature_c ||
                d[i].fanLevel_f    != tmpD[i].fanLevel_f)
                match = false;
        }
        if (match) return PRESET_IDX_CUSTOM;
    }
    return PRESET_IDX_NONE;
}

void ApplyPresetData(TemperaturePoint* handheld, TemperaturePoint* docked, bool* changed,
                     const TemperaturePoint* presetH, const TemperaturePoint* presetD)
{
    memcpy(handheld, presetH, sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    memcpy(docked,   presetD, sizeof(TemperaturePoint) * NUM_TEMP_POINTS);
    WriteFanConfig(handheld, docked);
    *changed = true;
    RestartSysmodule();
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

void RestartSysmodule()
{
    if (IsRunning() != 0) {
        pmshellTerminateProgram(SysFanControlID);
        const NcmProgramLocation loc{ .program_id = SysFanControlID, .storageID = NcmStorageId_None };
        u64 pid = 0;
        pmshellLaunchProgram(0, &loc, &pid);
    }
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
    if (pct < 20)  pct = 0;  // below hardware minimum = fan is off
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

// ── Shared monitoring ─────────────────────────────────────────────────────────

MonitoringState g_monitor;

bool UpdateMonitoring()
{
    u64 now = armGetSystemTick();
    if ((now - g_monitor.lastTick) < armNsToTicks(500'000'000ULL)) return false;
    g_monitor.lastTick = now;

    // Use raw apm service (command 1) like sys-clk — authoritative hardware mode.
    // apm:p (command 5, used by apmGetPerformanceMode) can lag on dock transitions
    // causing us to fall through to the PSM check and show "Official Charger" instead.
    bool isDocked = false;
    if (R_SUCCEEDED(g_apmRawCheck)) {
        u32 mode = 0;
        if (R_SUCCEEDED(serviceDispatchOut(&g_apmRawSrv, 1, mode)))
            isDocked = (mode != 0);
    } else {
        ApmPerformanceMode apmMode = ApmPerformanceMode_Normal;
        apmGetPerformanceMode(&apmMode);
        isDocked = (apmMode == ApmPerformanceMode_Boost);
    }

    if (isDocked) {
        g_monitor.profileStr = "Docked";
    } else {
        PsmChargerType chargerType = PsmChargerType_Unconnected;
        psmGetChargerType(&chargerType);
        if (chargerType == PsmChargerType_EnoughPower)
            g_monitor.profileStr = "Official Charger";
        else if (chargerType == PsmChargerType_LowPower)
            g_monitor.profileStr = "USB Charger";
        else
            g_monitor.profileStr = "Handheld";
    }

    g_monitor.fanPct   = ReadFanPercent();
    g_monitor.socTemp  = ReadSocTemp();
    g_monitor.pcbTemp  = ReadPcbTemp();
    g_monitor.skinTemp = ReadSkinTemp();

    if (g_monitor.fanPct >= 0)
        snprintf(g_monitor.fanBuf, sizeof(g_monitor.fanBuf), "%d%%", g_monitor.fanPct);
    else
        snprintf(g_monitor.fanBuf, sizeof(g_monitor.fanBuf), "--");

    if (g_monitor.socTemp >= 0.0f)
        snprintf(g_monitor.socTempBuf, sizeof(g_monitor.socTempBuf),
                 "%d.%d\xc2\xb0""C", (int)g_monitor.socTemp, (int)(g_monitor.socTemp * 10) % 10);
    if (g_monitor.pcbTemp >= 0.0f)
        snprintf(g_monitor.pcbTempBuf, sizeof(g_monitor.pcbTempBuf),
                 "%d.%d\xc2\xb0""C", (int)g_monitor.pcbTemp, (int)(g_monitor.pcbTemp * 10) % 10);
    if (g_monitor.skinTemp >= 0.0f)
        snprintf(g_monitor.skinTempBuf, sizeof(g_monitor.skinTempBuf),
                 "%d.%d\xc2\xb0""C", (int)g_monitor.skinTemp, (int)(g_monitor.skinTemp * 10) % 10);

    return true;
}

void DrawMonitoringHeader(tsl::gfx::Renderer* r, s32, s32, s32, s32,
                          const std::string& hintStr, const std::string& hintIcon)
{
    static s32   s_titleW      = -1;
    static s32   s_appLabelW   = -1;
    static s32   s_profLabelW  = -1;
    static s32   s_maxProfW    = -1;
    static s32   s_curveLabelW = -1;
    static s32   s_fanLabelW   = -1;
    static s32   s_pcbLabelW   = -1;
    static s32   s_skinLabelW  = -1;
    static float s_hintX       = -1.0f;

    if (s_titleW < 0) {
        s_titleW      = r->getTextDimensions("NX-FanControl",     false, 32).first;
        s_appLabelW   = r->getTextDimensions("App ID",            false, 15).first;
        s_profLabelW  = r->getTextDimensions("Profile",           false, 15).first;
        s_curveLabelW = r->getTextDimensions("Fan Curve Profile", false, 15).first;
        s_fanLabelW   = r->getTextDimensions("Fan",               false, 15).first;
        s_pcbLabelW   = r->getTextDimensions("PCB",               false, 15).first;
        s_skinLabelW  = r->getTextDimensions("Skin",              false, 15).first;
        s_maxProfW    = 0;
        for (const char* n : {"Docked", "Official Charger", "USB Charger", "Handheld"}) {
            s32 pw = r->getTextDimensions(n, false, 15).first;
            if (pw > s_maxProfW) s_maxProfW = pw;
        }
        const std::string stdFooter = "\xee\x83\xa1" + ult::GAP_2 + ult::BACK + ult::GAP_1 +
                                      "\xee\x83\xa0" + ult::GAP_2 + ult::OK   + ult::GAP_1;
        s_hintX = 30.0f + r->getTextDimensions(stdFooter, false, 23).first;
    }

    constexpr s32 ROW1_Y = 91;
    constexpr s32 ROW2_Y = 129;
    constexpr s32 ROW3_Y = 169;

    r->drawRoundedRect(14, 69, 420, 32, 10, r->aWithOpacity(tsl::tableBGColor));
    r->drawRoundedRect(14, 106, 420, 73, 10, r->aWithOpacity(tsl::tableBGColor));

    r->drawString("NX-FanControl", false, 20, 50, 32, tsl::defaultOverlayColor);
    r->drawString(APP_VERSION, false, 20 + s_titleW + 8, 50, 15, tsl::bannerVersionTextColor);

    // Row 1: App ID | Profile
    r->drawString("App ID",               false, 23,                    ROW1_Y, 15, tsl::sectionTextColor);
    r->drawString(g_monitor.appIdBuf,     false, 23 + s_appLabelW + 9,  ROW1_Y, 15, tsl::infoTextColor);
    {
        s32 valueX = 423 - s_maxProfW;
        r->drawString("Profile",                        false, valueX - 9 - s_profLabelW, ROW1_Y, 15, tsl::sectionTextColor);
        r->drawString(g_monitor.profileStr.c_str(),     false, valueX,                    ROW1_Y, 15, tsl::infoTextColor);
    }

    // Row 2: Fan Curve Profile | Fan %
    r->drawString("Fan Curve Profile",              false, 23,                      ROW2_Y, 15, tsl::sectionTextColor);
    r->drawString(g_monitor.curvePresetStr.c_str(), false, 23 + s_curveLabelW + 9,  ROW2_Y, 15, tsl::infoTextColor);
    r->drawString("Fan",                            false, 340 - 9 - s_fanLabelW,   ROW2_Y, 15, tsl::sectionTextColor);
    r->drawString(g_monitor.fanBuf,                 false, 340,                     ROW2_Y, 15, tsl::infoTextColor);

    // Row 3: Temps
    if (g_monitor.socTemp >= 0.0f) {
        r->drawString("SoC",                 false, 23,                ROW3_Y, 15, tsl::sectionTextColor);
        r->drawString(g_monitor.socTempBuf,  false, 63,                ROW3_Y, 15, tsl::GradientColor(g_monitor.socTemp));
    }
    if (g_monitor.pcbTemp >= 0.0f) {
        r->drawString("PCB",                 false, 192 - s_pcbLabelW, ROW3_Y, 15, tsl::sectionTextColor);
        r->drawString(g_monitor.pcbTempBuf,  false, 199,               ROW3_Y, 15, tsl::GradientColor(g_monitor.pcbTemp));
    }
    if (g_monitor.skinTemp >= 0.0f) {
        r->drawString("Skin",                false, 332 - s_skinLabelW, ROW3_Y, 15, tsl::sectionTextColor);
        r->drawString(g_monitor.skinTempBuf, false, 341,                ROW3_Y, 15, tsl::GradientColor(g_monitor.skinTemp));
    }

    if (!hintStr.empty())
        r->drawStringWithColoredSections(hintStr, false, {hintIcon},
                                         s_hintX, 693, 23,
                                         tsl::bottomTextColor, tsl::buttonColor);
}
