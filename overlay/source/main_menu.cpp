#include "main_menu.hpp"
#include "select_menu.hpp"
#include "settings_menu.hpp"
#include <cmath>

static std::string PLabel(int idx, TemperaturePoint* h, TemperaturePoint* d)
{
    return "P" + std::to_string(idx) + ": " +
           std::to_string(h[idx].temperature_c) + "C/" +
           std::to_string(d[idx].temperature_c) + "C | " +
           std::to_string((int)std::lround(h[idx].fanLevel_f * 100)) + "%/" +
           std::to_string((int)std::lround(d[idx].fanLevel_f * 100)) + "%";
}

MainMenu::MainMenu()
{
    ReadFanConfig(&this->_fanCurveHandheld, &this->_fanCurveDocked);
    snprintf(g_monitor.appIdBuf, sizeof(g_monitor.appIdBuf), "%016llX",
             (unsigned long long)0x0100000000001000ULL);
    snprintf(g_monitor.fanBuf, sizeof(g_monitor.fanBuf), "--");

    TemperaturePoint* h = this->_fanCurveHandheld;
    TemperaturePoint* d = this->_fanCurveDocked;
    {
        int preset = DetectCurvePreset(h, d);
        if (preset != PRESET_IDX_NONE)
            g_monitor.curvePresetStr = PresetIndexName(preset);
        else
            g_monitor.curvePresetStr = (access(CONFIG_FILE, F_OK) == 0) ? "Not Saved" : "Default";
    }
    for (int i = 0; i < NUM_TEMP_POINTS; i++)
        this->_pointLabels[i] = new tsl::elm::ListItem(PLabel(i, h, d));

    this->_fanCurveHeader = new tsl::elm::CategoryHeader("Fan Curve", true);
    this->_enabledBtn = new tsl::elm::ToggleListItem("Enabled", IsRunning() != 0);
}

tsl::elm::Element* MainMenu::createUI()
{
    this->_tableIsChanged = false;

    auto list = new tsl::elm::List();

    this->_enabledBtn->setStateChangedListener([this](bool state)
    {
        if (state)
        {
            CreateB2F();
            pmshellTerminateProgram(SysFanControlID);
            {
                const NcmProgramLocation programLocation{
                    .program_id = SysFanControlID,
                    .storageID = NcmStorageId_None,
                };
                u64 pid = 0;
                pmshellLaunchProgram(0, &programLocation, &pid);
            }
            return true;
        }
        else
        {
            RemoveB2F();
            if (IsRunning() != 0)
                pmshellTerminateProgram(SysFanControlID);
            return true;
        }
        return false;
    });
    list->addItem(this->_enabledBtn);
    list->addItem(this->_fanCurveHeader);

    auto addPoint = [&](int idx, tsl::elm::ListItem* label) {
        label->setClickListener([this, idx](uint64_t keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<SelectMenu>(idx, this->_fanCurveHandheld, this->_fanCurveDocked, &this->_tableIsChanged);
                return true;
            }
            return false;
        });
        list->addItem(label);
    };

    for (int i = 0; i < NUM_TEMP_POINTS; i++)
        addPoint(i, this->_pointLabels[i]);

    auto* frame = new tsl::elm::HeaderOverlayFrame(191);

    static const std::string s_hint = "\xee\x83\xae" + ult::GAP_2 + "Settings";
    frame->setHeader(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        DrawMonitoringHeader(r, x, y, w, h, s_hint, "\xee\x83\xae");
    }));

    frame->setContent(list);
    return frame;
}

void MainMenu::update()
{
    if (UpdateMonitoring()) {
        // Title ID — game profile switching
        u64 currentTitleId = GetRunningTitleId();
        if (currentTitleId != this->_lastTitleId) {
            this->_lastTitleId = currentTitleId;
            u64 displayTid = currentTitleId != 0 ? currentTitleId : 0x0100000000001000ULL;
            snprintf(g_monitor.appIdBuf, sizeof(g_monitor.appIdBuf),
                     "%016llX", (unsigned long long)displayTid);
            if (currentTitleId != 0) {
                int preset = GetGameProfile(currentTitleId);
                const FanPreset* presetTable = PresetIndexTable(preset);
                if (presetTable) {
                    ApplyPresetData(this->_fanCurveHandheld, this->_fanCurveDocked,
                                    &this->_tableIsChanged,
                                    presetTable->handheld, presetTable->docked);
                } else if (preset == PRESET_IDX_CUSTOM) {
                    TemperaturePoint tmpH[NUM_TEMP_POINTS], tmpD[NUM_TEMP_POINTS];
                    if (LoadCustomCurve(tmpH, tmpD))
                        ApplyPresetData(this->_fanCurveHandheld, this->_fanCurveDocked,
                                        &this->_tableIsChanged, tmpH, tmpD);
                }
            }
        }
    }

    if (this->_tableIsChanged) {
        TemperaturePoint* hh = this->_fanCurveHandheld;
        TemperaturePoint* dd = this->_fanCurveDocked;
        for (int i = 0; i < NUM_TEMP_POINTS; i++)
            this->_pointLabels[i]->setText(PLabel(i, hh, dd));
        int preset = DetectCurvePreset(hh, dd);
        g_monitor.curvePresetStr = (preset == PRESET_IDX_NONE) ? "Not Saved" : PresetIndexName(preset);
        this->_tableIsChanged = false;
    }
}

bool MainMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                            HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick)
{
    if (keysDown & HidNpadButton_Right) {
        tsl::changeTo<SettingsMenu>(this->_fanCurveHandheld, this->_fanCurveDocked, &this->_tableIsChanged);
        return true;
    }
    return false;
}
