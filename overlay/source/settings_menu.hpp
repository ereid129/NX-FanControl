#pragma once
#include <tesla.hpp>
#include <fancontrol.h>
#include "utils.hpp"

class SettingsMenu : public tsl::Gui {
private:
    TemperaturePoint*          _fanCurveHandheld;
    TemperaturePoint*          _fanCurveDocked;
    bool*                      _tableIsChanged;

    u64                        _currentTitleId  = 0;
    int                        _currentGamePreset = PRESET_IDX_NONE;
    tsl::elm::ListItem*        _gamePresetItem  = nullptr;

public:
    SettingsMenu(TemperaturePoint* handheld, TemperaturePoint* docked, bool* tableIsChanged);
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
