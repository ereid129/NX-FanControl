#pragma once
#include <tesla.hpp>
#include "utils.hpp"

class SelectMenu : public tsl::Gui {
private:
    int               _i = 0;
    TemperaturePoint* _fanCurveTable;
    bool*             _tableIsChanged;

    tsl::elm::CategoryHeader* _tempLabel;
    tsl::elm::CategoryHeader* _fanLabel;
    tsl::elm::ListItem*       _saveBtn;

    tsl::elm::StepTrackBar*   _stepTemp = nullptr;
    tsl::elm::StepTrackBar*   _stepFan  = nullptr;

    int _rawTemp    = 0;   // actual °C value (0–100)
    int _rawFan     = 0;   // actual % value (0–100)
    int _focusIndex = 0;   // 0=temp slider, 1=fan slider, 2=save btn

public:
    SelectMenu(int i, TemperaturePoint* tps, bool* tableIsChanged);

    virtual tsl::elm::Element* createUI() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
