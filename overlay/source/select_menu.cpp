#include "select_menu.hpp"
#include <algorithm>
#include <cmath>

SelectMenu::SelectMenu(int i, TemperaturePoint* handheld, TemperaturePoint* docked, bool* tableIsChanged)
{
    this->_i              = i;
    this->_handheld       = handheld;
    this->_docked         = docked;
    this->_tableIsChanged = tableIsChanged;

    this->_rawTemp  = handheld[i].temperature_c;
    this->_rawTempD = docked[i].temperature_c;
    this->_rawFanH  = (int)std::lround(handheld[i].fanLevel_f * 100);
    this->_rawFanD  = (int)std::lround(docked[i].fanLevel_f * 100);

    this->_saveBtn          = new tsl::elm::ListItem("Save");
    this->_tempLabel        = new tsl::elm::CategoryHeader("Handheld: " + std::to_string(this->_rawTemp)  + "C", true);
    this->_tempDockedLabel  = new tsl::elm::CategoryHeader("Docked: "   + std::to_string(this->_rawTempD) + "C", true);
    this->_fanHandheldLabel = new tsl::elm::CategoryHeader("Handheld: " + std::to_string(this->_rawFanH)  + "%", true);
    this->_fanDockedLabel   = new tsl::elm::CategoryHeader("Docked: "   + std::to_string(this->_rawFanD)  + "%", true);
}

tsl::elm::Element* SelectMenu::createUI()
{
    auto list = new tsl::elm::List();

    // ── Temperature ──────────────────────────────────────────────────────────
    list->addItem(this->_tempLabel);
    this->_stepTemp = new TicklessStepTrackBar("C", 21);
    this->_stepTemp->setValueChangedListener([this](u8 value) {
        this->_rawTemp = value * 5;
        this->_tempLabel->setText("Handheld: " + std::to_string(this->_rawTemp) + "C");
        this->_handheld[this->_i].temperature_c = this->_rawTemp;
        this->_saveBtn->setText("Save");
    });
    this->_stepTemp->setProgress(this->_rawTemp / 5);
    list->addItem(this->_stepTemp);

    list->addItem(this->_tempDockedLabel);
    this->_stepTempDocked = new TicklessStepTrackBar("C", 21);
    this->_stepTempDocked->setValueChangedListener([this](u8 value) {
        this->_rawTempD = value * 5;
        this->_tempDockedLabel->setText("Docked: " + std::to_string(this->_rawTempD) + "C");
        this->_docked[this->_i].temperature_c = this->_rawTempD;
        this->_saveBtn->setText("Save");
    });
    this->_stepTempDocked->setProgress(this->_rawTempD / 5);
    list->addItem(this->_stepTempDocked);

    // ── Fan speed ─────────────────────────────────────────────────────────────
    list->addItem(this->_fanHandheldLabel);
    this->_stepFanHandheld = new TicklessStepTrackBar("%", 21);
    this->_stepFanHandheld->setValueChangedListener([this](u8 value) {
        int pct = value * 5;
        if (pct > 0 && pct < FAN_MIN_PCT) pct = FAN_MIN_PCT;
        this->_rawFanH = pct;
        this->_fanHandheldLabel->setText("Handheld: " + std::to_string(this->_rawFanH) + "%");
        this->_handheld[this->_i].fanLevel_f = (float)this->_rawFanH / 100.0f;
        this->_saveBtn->setText("Save");
    });
    this->_stepFanHandheld->setProgress(this->_rawFanH / 5);
    list->addItem(this->_stepFanHandheld);

    list->addItem(this->_fanDockedLabel);
    this->_stepFanDocked = new TicklessStepTrackBar("%", 21);
    this->_stepFanDocked->setValueChangedListener([this](u8 value) {
        int pct = value * 5;
        if (pct > 0 && pct < FAN_MIN_PCT) pct = FAN_MIN_PCT;
        this->_rawFanD = pct;
        this->_fanDockedLabel->setText("Docked: " + std::to_string(this->_rawFanD) + "%");
        this->_docked[this->_i].fanLevel_f = (float)this->_rawFanD / 100.0f;
        this->_saveBtn->setText("Save");
    });
    this->_stepFanDocked->setProgress(this->_rawFanD / 5);
    list->addItem(this->_stepFanDocked);

    this->_saveBtn->setClickListener([this](uint64_t keys) {
        if (keys & HidNpadButton_A) {
            WriteFanConfig(this->_handheld, this->_docked);
            RestartSysmodule();
            this->_saveBtn->setText("Saved!");
            *this->_tableIsChanged = true;
            return true;
        }
        return false;
    });
    list->addItem(this->_saveBtn);

    auto frame = new tsl::elm::HeaderOverlayFrame(191);
    frame->setHeader(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer* r, s32 x, s32 y, s32 w, s32 h) {
        DrawMonitoringHeader(r, x, y, w, h, "", "");
    }));
    frame->setContent(list);
    return frame;
}

void SelectMenu::update()
{
    UpdateMonitoring();
}

bool SelectMenu::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                              HidAnalogStickState leftJoyStick, HidAnalogStickState rightJoyStick)
{
    // R held = fine ±1 adjustment only; block Up/Down so focus can't drift
    if (keysHeld & HidNpadButton_R) {
        if (keysDown & (HidNpadButton_Up | HidNpadButton_Down))
            return true;

        int delta = 0;
        if (keysDown & HidNpadButton_Right) delta = +1;
        if (keysDown & HidNpadButton_Left)  delta = -1;

        if (delta != 0 && this->_focusIndex == 0) {
            this->_rawTemp = std::max(0, std::min(100, this->_rawTemp + delta));
            this->_tempLabel->setText("Handheld: " + std::to_string(this->_rawTemp) + "C");
            this->_handheld[this->_i].temperature_c = this->_rawTemp;
            this->_stepTemp->setProgress(this->_rawTemp / 5);
            this->_saveBtn->setText("Save");
            return true;
        }
        if (delta != 0 && this->_focusIndex == 1) {
            this->_rawTempD = std::max(0, std::min(100, this->_rawTempD + delta));
            this->_tempDockedLabel->setText("Docked: " + std::to_string(this->_rawTempD) + "C");
            this->_docked[this->_i].temperature_c = this->_rawTempD;
            this->_stepTempDocked->setProgress(this->_rawTempD / 5);
            this->_saveBtn->setText("Save");
            return true;
        }
        if (delta != 0 && this->_focusIndex == 2) {
            this->_rawFanH = std::max(0, std::min(100, this->_rawFanH + delta));
            if (this->_rawFanH > 0 && this->_rawFanH < FAN_MIN_PCT)
                this->_rawFanH = (delta > 0) ? FAN_MIN_PCT : 0;
            this->_fanHandheldLabel->setText("Handheld: " + std::to_string(this->_rawFanH) + "%");
            this->_handheld[this->_i].fanLevel_f = (float)this->_rawFanH / 100.0f;
            this->_stepFanHandheld->setProgress(this->_rawFanH / 5);
            this->_saveBtn->setText("Save");
            return true;
        }
        if (delta != 0 && this->_focusIndex == 3) {
            this->_rawFanD = std::max(0, std::min(100, this->_rawFanD + delta));
            if (this->_rawFanD > 0 && this->_rawFanD < FAN_MIN_PCT)
                this->_rawFanD = (delta > 0) ? FAN_MIN_PCT : 0;
            this->_fanDockedLabel->setText("Docked: " + std::to_string(this->_rawFanD) + "%");
            this->_docked[this->_i].fanLevel_f = (float)this->_rawFanD / 100.0f;
            this->_stepFanDocked->setProgress(this->_rawFanD / 5);
            this->_saveBtn->setText("Save");
            return true;
        }
    }

    // Up/Down (no R): track which slider R+left/right will target
    if (keysDown & HidNpadButton_Down)
        this->_focusIndex = std::min(this->_focusIndex + 1, 3);
    if (keysDown & HidNpadButton_Up)
        this->_focusIndex = std::max(this->_focusIndex - 1, 0);

    return false;
}
