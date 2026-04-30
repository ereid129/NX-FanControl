#pragma once
#include <tesla.hpp>
#include "utils.hpp"

class TicklessStepTrackBar : public tsl::elm::StepTrackBar {
public:
    TicklessStepTrackBar(const char icon[3], size_t numSteps)
        : tsl::elm::StepTrackBar(icon, numSteps) {
        m_usingStepTrackbar = false;
    }
};

class SelectMenu : public tsl::Gui {
private:
    int               _i = 0;
    TemperaturePoint* _handheld;
    TemperaturePoint* _docked;
    bool*             _tableIsChanged;

    tsl::elm::CategoryHeader* _tempLabel;
    tsl::elm::CategoryHeader* _tempDockedLabel;
    tsl::elm::CategoryHeader* _fanHandheldLabel;
    tsl::elm::CategoryHeader* _fanDockedLabel;
    tsl::elm::ListItem*       _saveBtn;

    TicklessStepTrackBar* _stepTemp        = nullptr;
    TicklessStepTrackBar* _stepTempDocked  = nullptr;
    TicklessStepTrackBar* _stepFanHandheld = nullptr;
    TicklessStepTrackBar* _stepFanDocked   = nullptr;

    int _rawTemp       = 0;
    int _rawTempD      = 0;
    int _rawFanH       = 0;
    int _rawFanD       = 0;
    int _focusIndex    = 0;   // 0=H temp, 1=D temp, 2=H fan, 3=D fan

public:
    SelectMenu(int i, TemperaturePoint* handheld, TemperaturePoint* docked, bool* tableIsChanged);

    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
