#include "select_menu.hpp"
#include <algorithm>

SelectMenu::SelectMenu(int i, TemperaturePoint* fanCurveTable, bool* tableIsChanged)
{
    this->_i              = i;
    this->_fanCurveTable  = fanCurveTable;
    this->_tableIsChanged = tableIsChanged;

    this->_rawTemp = (this->_fanCurveTable + i)->temperature_c;
    this->_rawFan  = (int)((this->_fanCurveTable + i)->fanLevel_f * 100);

    this->_saveBtn  = new tsl::elm::ListItem("Save");
    this->_tempLabel = new tsl::elm::CategoryHeader(std::to_string(this->_rawTemp) + "C", true);
    this->_fanLabel  = new tsl::elm::CategoryHeader(std::to_string(this->_rawFan)  + "%", true);
}

tsl::elm::Element* SelectMenu::createUI()
{
    auto list = new tsl::elm::List();

    list->addItem(this->_tempLabel);
    this->_stepTemp = new tsl::elm::StepTrackBar("C", 21);
    this->_stepTemp->setValueChangedListener([this](u8 value) {
        this->_rawTemp = value * 5;
        this->_tempLabel->setText(std::to_string(this->_rawTemp) + "C");
        (this->_fanCurveTable + this->_i)->temperature_c = this->_rawTemp;
        this->_saveBtn->setText("Save");
    });
    this->_stepTemp->setProgress(this->_rawTemp / 5);
    list->addItem(this->_stepTemp);

    list->addItem(this->_fanLabel);
    this->_stepFan = new tsl::elm::StepTrackBar("%", 21);
    this->_stepFan->setValueChangedListener([this](u8 value) {
        this->_rawFan = value * 5;
        this->_fanLabel->setText(std::to_string(this->_rawFan) + "%");
        (this->_fanCurveTable + this->_i)->fanLevel_f = (float)this->_rawFan / 100.0f;
        this->_saveBtn->setText("Save");
    });
    this->_stepFan->setProgress(this->_rawFan / 5);
    list->addItem(this->_stepFan);

    this->_saveBtn->setClickListener([this](uint64_t keys) {
        if (keys & HidNpadButton_A) {
            WriteConfigFile(this->_fanCurveTable);
            if (IsRunning() != 0) {
                pmshellTerminateProgram(SysFanControlID);
                const NcmProgramLocation programLocation{
                    .program_id = SysFanControlID,
                    .storageID  = NcmStorageId_None,
                };
                u64 pid = 0;
                pmshellLaunchProgram(0, &programLocation, &pid);
            }
            this->_saveBtn->setText("Saved!");
            *this->_tableIsChanged = true;
            return true;
        }
        return false;
    });
    list->addItem(this->_saveBtn);

    auto frame = new tsl::elm::OverlayFrame("NX-FanControl", APP_VERSION);
    frame->setContent(list);
    return frame;
}

bool SelectMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                              HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick)
{
    // Track which item is focused (temp slider=0, fan slider=1, save=2)
    if (keysDown & HidNpadButton_Down)
        this->_focusIndex = std::min(this->_focusIndex + 1, 2);
    if (keysDown & HidNpadButton_Up)
        this->_focusIndex = std::max(this->_focusIndex - 1, 0);

    // R held = fine ±1 adjustment on the focused slider
    if (keysHeld & HidNpadButton_R) {
        int delta = 0;
        if (keysDown & HidNpadButton_Right) delta = +1;
        if (keysDown & HidNpadButton_Left)  delta = -1;

        if (delta != 0 && this->_focusIndex == 0) {
            this->_rawTemp = std::max(0, std::min(100, this->_rawTemp + delta));
            this->_tempLabel->setText(std::to_string(this->_rawTemp) + "C");
            (this->_fanCurveTable + this->_i)->temperature_c = this->_rawTemp;
            this->_saveBtn->setText("Save");
            return true;
        }
        if (delta != 0 && this->_focusIndex == 1) {
            this->_rawFan = std::max(0, std::min(100, this->_rawFan + delta));
            this->_fanLabel->setText(std::to_string(this->_rawFan) + "%");
            (this->_fanCurveTable + this->_i)->fanLevel_f = (float)this->_rawFan / 100.0f;
            this->_saveBtn->setText("Save");
            return true;
        }
    }

    return false;
}
