#pragma once
#include <tesla.hpp>
#include <fancontrol.h>
#include "utils.hpp"

class HideableListItem : public tsl::elm::ListItem {
    bool _visible = true;
public:
    HideableListItem(const std::string& text) : tsl::elm::ListItem(text) {}

    void setVisible(bool v) {
        if (_visible == v) return;
        _visible = v;
        m_isItem = v;
        if (this->getParent() != nullptr)
            this->getParent()->invalidate();
    }

    virtual tsl::elm::Element* requestFocus(tsl::elm::Element* oldFocus, tsl::FocusDirection dir) override {
        return _visible ? tsl::elm::ListItem::requestFocus(oldFocus, dir) : nullptr;
    }

    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        if (_visible)
            tsl::elm::ListItem::layout(parentX, parentY, parentWidth, parentHeight);
        else
            this->setBoundaries(this->getX(), this->getY(), this->getWidth(), 0);
    }

    virtual void draw(tsl::gfx::Renderer* r) override {
        if (_visible) tsl::elm::ListItem::draw(r);
    }
};

class MainMenu : public tsl::Gui {
private:
    TemperaturePoint*         _fanCurveTable;
    bool                      _tableIsChanged;
    u64                       _lastTitleId = 0;

    tsl::elm::ToggleListItem* _enabledBtn;

    tsl::elm::ListItem*       _fanSpeedLabel;
    HideableListItem*         _socTempLabel;
    HideableListItem*         _pcbTempLabel;
    HideableListItem*         _skinTempLabel;

    tsl::elm::ListItem*       _p0Label;
    tsl::elm::ListItem*       _p1Label;
    tsl::elm::ListItem*       _p2Label;
    tsl::elm::ListItem*       _p3Label;
    tsl::elm::ListItem*       _p4Label;

public:
    MainMenu();
    virtual tsl::elm::Element* createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
                             HidAnalogStickState leftJoyStick,
                             HidAnalogStickState rightJoyStick) override;
};
