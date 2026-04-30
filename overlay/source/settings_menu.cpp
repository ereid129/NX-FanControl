#include "settings_menu.hpp"

SettingsMenu::SettingsMenu(TemperaturePoint* handheld, TemperaturePoint* docked, bool* tableIsChanged)
    : _fanCurveHandheld(handheld), _fanCurveDocked(docked), _tableIsChanged(tableIsChanged)
{
}

tsl::elm::Element* SettingsMenu::createUI()
{
    _currentTitleId    = GetRunningTitleId();
    _currentGamePreset = GetGameProfile(_currentTitleId);

    auto list = new tsl::elm::List();

    // ── Game Profiles ────────────────────────────────────────────────────────
    list->addItem(new tsl::elm::CategoryHeader("Game Profiles", true));

    _gamePresetItem = new tsl::elm::ListItem("Game Preset");
    _gamePresetItem->setValue(PresetIndexName(_currentGamePreset));
    _gamePresetItem->setClickListener([this](uint64_t keys) -> bool {
        if (!(keys & HidNpadButton_A)) return false;
        if (this->_currentTitleId == 0) return true;

        this->_currentGamePreset = (this->_currentGamePreset + 1) % (PRESET_IDX_CUSTOM + 1);
        SetGameProfile(this->_currentTitleId, this->_currentGamePreset);
        this->_gamePresetItem->setValue(PresetIndexName(this->_currentGamePreset));

        const FanPreset* tbl = PresetIndexTable(this->_currentGamePreset);
        if (tbl) {
            ApplyPresetData(this->_fanCurveHandheld, this->_fanCurveDocked,
                            this->_tableIsChanged, tbl->handheld, tbl->docked);
        } else if (this->_currentGamePreset == PRESET_IDX_CUSTOM) {
            TemperaturePoint tmpH[NUM_TEMP_POINTS], tmpD[NUM_TEMP_POINTS];
            if (LoadCustomCurve(tmpH, tmpD))
                ApplyPresetData(this->_fanCurveHandheld, this->_fanCurveDocked,
                                this->_tableIsChanged, tmpH, tmpD);
        }
        return true;
    });
    list->addItem(_gamePresetItem);

    auto clearProfileItem = new tsl::elm::ListItem("Clear Game Profile");
    clearProfileItem->setClickListener([this](uint64_t keys) -> bool {
        if (!(keys & HidNpadButton_A)) return false;
        if (this->_currentTitleId == 0) return true;
        RemoveGameProfile(this->_currentTitleId);
        this->_currentGamePreset = PRESET_IDX_NONE;
        this->_gamePresetItem->setValue(PresetIndexName(PRESET_IDX_NONE));
        return true;
    });
    list->addItem(clearProfileItem);

    // ── Fan Curve ────────────────────────────────────────────────────────────
    list->addItem(new tsl::elm::CategoryHeader("Fan Curve", true));

    struct { const char* name; const FanPreset* preset; } builtinPresets[] = {
        { "Silent",      &PRESET_SILENT      },
        { "Balanced",    &PRESET_BALANCED    },
        { "Performance", &PRESET_PERFORMANCE },
    };
    for (auto& p : builtinPresets) {
        auto item = new tsl::elm::ListItem(p.name);
        const FanPreset* preset = p.preset;
        item->setClickListener([this, preset](uint64_t keys) -> bool {
            if (keys & HidNpadButton_A) {
                ApplyPresetData(_fanCurveHandheld, _fanCurveDocked, _tableIsChanged,
                                preset->handheld, preset->docked);
                return true;
            }
            return false;
        });
        list->addItem(item);
    }

    auto customItem = new tsl::elm::ListItem("Custom");
    customItem->setValue(access(CUSTOM_CURVE_PATH, F_OK) == 0 ? "" : "None saved");
    customItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) {
            TemperaturePoint tmpH[NUM_TEMP_POINTS], tmpD[NUM_TEMP_POINTS];
            if (LoadCustomCurve(tmpH, tmpD))
                ApplyPresetData(this->_fanCurveHandheld, this->_fanCurveDocked,
                                this->_tableIsChanged, tmpH, tmpD);
            return true;
        }
        return false;
    });
    list->addItem(customItem);

    auto saveItem = new tsl::elm::ListItem("Save as Custom");
    saveItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) {
            SaveCustomCurve(this->_fanCurveHandheld, this->_fanCurveDocked);
            return true;
        }
        return false;
    });
    list->addItem(saveItem);

    static const std::string s_hint = "" + ult::GAP_2 + "Main Menu";
    auto frame = new tsl::elm::HeaderOverlayFrame(191);
    frame->setHeader(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        DrawMonitoringHeader(r, x, y, w, h, s_hint, "");
    }));
    frame->setContent(list);
    return frame;
}

void SettingsMenu::update()
{
    UpdateMonitoring();
}

bool SettingsMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                                HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick)
{
    if (keysDown & HidNpadButton_Left) {
        tsl::goBack();
        return true;
    }
    return false;
}
