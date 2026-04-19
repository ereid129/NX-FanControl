#include "settings_menu.hpp"
#include <cstdio>

extern UiSettings* g_uiSettings;

static std::string titleIdStr(u64 id)
{
    if (id == 0) return "None";
    char buf[17];
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)id);
    return std::string(buf);
}

SettingsMenu::SettingsMenu(TemperaturePoint* table, bool* tableIsChanged)
    : _fanCurveTable(table), _tableIsChanged(tableIsChanged)
{
    _socTempToggle  = new tsl::elm::ToggleListItem("Show SoC Temp",  g_uiSettings->showSocTemp  != 0);
    _pcbTempToggle  = new tsl::elm::ToggleListItem("Show PCB Temp",  g_uiSettings->showPcbTemp  != 0);
    _skinTempToggle = new tsl::elm::ToggleListItem("Show Skin Temp", g_uiSettings->showSkinTemp != 0);
}

tsl::elm::Element* SettingsMenu::createUI()
{
    _currentTitleId    = GetRunningTitleId();
    _currentGamePreset = GetGameProfile(_currentTitleId);

    auto list = new tsl::elm::List();

    // ── Temperature Display ──────────────────────────────────────────────────
    list->addItem(new tsl::elm::CategoryHeader("Temperature Display", true));

    _socTempToggle->setStateChangedListener([](bool state) -> bool {
        g_uiSettings->showSocTemp = state ? 1 : 0;
        SaveUiSettings(g_uiSettings);
        return true;
    });
    list->addItem(_socTempToggle);

    _pcbTempToggle->setStateChangedListener([](bool state) -> bool {
        g_uiSettings->showPcbTemp = state ? 1 : 0;
        SaveUiSettings(g_uiSettings);
        return true;
    });
    list->addItem(_pcbTempToggle);

    _skinTempToggle->setStateChangedListener([](bool state) -> bool {
        g_uiSettings->showSkinTemp = state ? 1 : 0;
        SaveUiSettings(g_uiSettings);
        return true;
    });
    list->addItem(_skinTempToggle);

    // ── Game Profiles ────────────────────────────────────────────────────────
    list->addItem(new tsl::elm::CategoryHeader("Game Profiles", true));

    auto gameInfoItem = new tsl::elm::ListItem("Current Game");
    gameInfoItem->setValue(titleIdStr(_currentTitleId));
    list->addItem(gameInfoItem);

    _gamePresetItem = new tsl::elm::ListItem("Game Preset");
    _gamePresetItem->setValue(PresetIndexName(_currentGamePreset));
    _gamePresetItem->setClickListener([this](uint64_t keys) -> bool {
        if (!(keys & HidNpadButton_A)) return false;
        if (this->_currentTitleId == 0) return true;

        // cycle: None → Silent → Balanced → Performance → Custom → None
        this->_currentGamePreset = (this->_currentGamePreset + 1) % (PRESET_IDX_CUSTOM + 1);
        SetGameProfile(this->_currentTitleId, this->_currentGamePreset);
        this->_gamePresetItem->setValue(PresetIndexName(this->_currentGamePreset));

        // apply immediately
        const TemperaturePoint* tbl = PresetIndexTable(this->_currentGamePreset);
        if (tbl) {
            ApplyPresetData(this->_fanCurveTable, this->_tableIsChanged, tbl);
        } else if (this->_currentGamePreset == PRESET_IDX_CUSTOM) {
            TemperaturePoint tmp[5];
            if (LoadCustomCurve(tmp))
                ApplyPresetData(this->_fanCurveTable, this->_tableIsChanged, tmp);
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

    // ── Fan Curve Presets ────────────────────────────────────────────────────
    list->addItem(new tsl::elm::CategoryHeader("Fan Curve", true));

    auto saveItem = new tsl::elm::ListItem("Save as Custom");
    saveItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) { SaveCustomCurve(this->_fanCurveTable); return true; }
        return false;
    });
    list->addItem(saveItem);

    auto customItem = new tsl::elm::ListItem("Custom");
    customItem->setValue(access(CUSTOM_CURVE_PATH, F_OK) == 0 ? "" : "None saved");
    customItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) {
            TemperaturePoint tmp[5];
            if (LoadCustomCurve(tmp))
                ApplyPresetData(this->_fanCurveTable, this->_tableIsChanged, tmp);
            return true;
        }
        return false;
    });
    list->addItem(customItem);

    auto silentItem = new tsl::elm::ListItem("Silent");
    silentItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) { ApplyPresetData(_fanCurveTable, _tableIsChanged, PRESET_SILENT); return true; }
        return false;
    });
    list->addItem(silentItem);

    auto balancedItem = new tsl::elm::ListItem("Balanced");
    balancedItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) { ApplyPresetData(_fanCurveTable, _tableIsChanged, PRESET_BALANCED); return true; }
        return false;
    });
    list->addItem(balancedItem);

    auto perfItem = new tsl::elm::ListItem("Performance");
    perfItem->setClickListener([this](uint64_t keys) -> bool {
        if (keys & HidNpadButton_A) { ApplyPresetData(_fanCurveTable, _tableIsChanged, PRESET_PERFORMANCE); return true; }
        return false;
    });
    list->addItem(perfItem);

    auto frame = new tsl::elm::OverlayFrame("Settings", APP_VERSION, false, "Back", "");
    frame->setContent(list);
    return frame;
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
