#pragma once
#include <tesla.hpp>
#include <fancontrol.h>
#include "utils.hpp"

class SettingsMenu : public tsl::Gui {
private:
    TemperaturePoint*          _fanCurveTable;
    bool*                      _tableIsChanged;

    tsl::elm::ToggleListItem*  _socTempToggle;
    tsl::elm::ToggleListItem*  _pcbTempToggle;
    tsl::elm::ToggleListItem*  _skinTempToggle;

    u64                        _currentTitleId  = 0;
    int                        _currentGamePreset = PRESET_IDX_NONE;
    tsl::elm::ListItem*        _gamePresetItem  = nullptr;

public:
    SettingsMenu(TemperaturePoint* table, bool* tableIsChanged);
    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
