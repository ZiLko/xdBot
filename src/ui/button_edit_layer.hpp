#pragma once

#include "../includes.hpp"

const std::vector<std::string> indexToID = {
    "button_off", "button_advance_frame", "button_speedhack"
};

const std::map<std::string, int> IDtoIndex{
    {"button_off", 0}, {"button_advance_frame", 1}, {"button_speedhack", 2}
};

const std::map<std::string, std::string> IDtoName{
    {"button_off", "Frame Stepper Off"}, {"button_advance_frame", "Advance Frame"}, {"button_speedhack", "Toggle Speedhack"}
};

struct MovingButton {
    size_t index = 0;
    CCSprite* sprite = nullptr;
    cocos2d::CCPoint offset = ccp(0, 0);
};

class ButtonEditLayer : public geode::Popup<> {

private:
    
    bool setup() override;

public:

    STATIC_CREATE(ButtonEditLayer, 200, 131)
    
    Mod* mod = nullptr;
    CCMenu* menu = nullptr;

    Slider* scaleSlider = nullptr;
    Slider* opacitySlider = nullptr;

    CCLabelBMFont* scaleLbl = nullptr;
    CCLabelBMFont* opacityLbl = nullptr;
    CCLabelBMFont* selectedLbl = nullptr;

    std::vector<CCSprite*> spriteButtons;

    MovingButton movingButton;

    size_t currentSelected = 0;

    std::map<std::string, cocos2d::CCPoint> positions;
    std::map<std::string, float> scales;
    std::map<std::string, float> opacities;

    void onRestore(CCObject*);

    void onSave(CCObject*);

    void updateScale(CCObject*);

    void updateOpacity(CCObject*);

    void updateSelectedLabels();

    void updateSelected(std::string selected);

    void addSliders();

    void addSprites();

    static bool isPointInButton(cocos2d::CCPoint clickPos, cocos2d::CCPoint btnPos, cocos2d::CCSize btnSize);

};