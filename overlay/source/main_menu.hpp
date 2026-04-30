#pragma once
#include <tesla.hpp>
#include <fancontrol.h>
#include "utils.hpp"

class MainMenu : public tsl::Gui {
private:
    TemperaturePoint*         _fanCurveHandheld;
    TemperaturePoint*         _fanCurveDocked;
    bool                      _tableIsChanged;
    u64                       _lastTitleId = 0;


    tsl::elm::ToggleListItem* _enabledBtn;
    tsl::elm::CategoryHeader* _fanCurveHeader;
    tsl::elm::ListItem*       _pointLabels[NUM_TEMP_POINTS];

public:
    MainMenu();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
