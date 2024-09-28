#include "../includes.hpp"
#include "game_ui.hpp"

#include <Geode/modify/PlayLayer.hpp>

class $modify(PlayLayer) {

    struct Fields {
        CCLabelBMFont* frameLabel = nullptr;
    };

    static void onModify(auto & self) {
        if (!self.setHookPriority("PlayLayer::init", -1))
            log::warn("PlayLayer::init hook priority fail xD.");
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        auto& g = Global::get();

        if (g.state != state::none && g.frameLabel && !g.renderer.recording)
            m_fields->frameLabel->setString(("Frame: " + std::to_string(Global::getCurrentFrame())).c_str());
    }

    bool init(GJGameLevel * level, bool b1, bool b2) {
        if (!PlayLayer::init(level, b1, b2)) return false;

        Interface::addLabels(this);
        Interface::addButtons(this);

        m_fields->frameLabel = static_cast<CCLabelBMFont*>(getChildByID("frame-label"_spr));

        return true;
    }
};

void Interface::addLabels(PlayLayer* pl) {
    CCLabelBMFont* stateLabel = CCLabelBMFont::create("", "chatFont.fnt");
    stateLabel->setPosition({ CCDirector::sharedDirector()->getWinSize().width - 6.5f, 12 });
    stateLabel->setAnchorPoint({ 1, 0.5 });
    stateLabel->setID("state-label"_spr);
    stateLabel->setZOrder(300);
    stateLabel->setScale(0.625f);
    pl->addChild(stateLabel);

    CCLabelBMFont* frameLabel = CCLabelBMFont::create("", "chatFont.fnt");
    frameLabel->setPosition({ 6.5f, 12 });
    frameLabel->setAnchorPoint({ 0, 0.5 });
    frameLabel->setID("frame-label"_spr);
    frameLabel->setZOrder(300);
    frameLabel->setScale(0.625f);
    pl->addChild(frameLabel);

    Interface::updateLabels();
}

void Interface::addButtons(PlayLayer* pl) {
    cocos2d::CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    CCMenu* menu = CCMenu::create();
    menu->setZOrder(300);
    menu->setPosition({ 0, 0 });
    menu->setID("button-menu"_spr);
    pl->addChild(menu);

    CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    spr->setFlipX(true);

    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, pl, menu_selector(Interface::onFrameStepper));
    btn->setAnchorPoint({ 0, 0 });
    btn->setID("step-frame-btn");
    CCSprite* sprite = getChildOfType<CCSprite>(btn, 0);
    sprite->setPosition({ 0, 0 });

    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");

    btn = CCMenuItemSpriteExtra::create(spr, pl, menu_selector(Interface::onFrameStepperOff));
    btn->setID("disable-stepper-btn");
    btn->setAnchorPoint({ 0, 0 });
    sprite = getChildOfType<CCSprite>(btn, 0);
    sprite->setPosition({ 0, 0 });

    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");

    btn = CCMenuItemSpriteExtra::create(spr, pl, menu_selector(Interface::onSpeedhack));
    btn->setAnchorPoint({ 0, 0 });
    btn->setID("speedhack-btn");
    sprite = getChildOfType<CCSprite>(btn, 0);
    sprite->setPosition({ 0, 0 });

    menu->addChild(btn);

    Interface::updateButtons();
}

void Interface::updateLabels() {
    PlayLayer* pl = PlayLayer::get();
    auto& g = Global::get();

    if (!pl) return;

    if (g.state == state::none || !g.frameLabel)
        static_cast<CCLabelBMFont*>(pl->getChildByID("frame-label"_spr))->setString("");

    CCLabelBMFont* label = typeinfo_cast<CCLabelBMFont*>(pl->getChildByID("state-label"_spr));

    if (!label) return;

    if (g.mod->getSavedValue<bool>("macro_hide_labels"))
        return label->setString("");

    state state = g.state;
    std::string labelText = state == state::none ? "" : "Playing";
    if (state == state::recording)
        labelText = "Recording";

    if (labelText == "Recording" && state == state::recording && g.mod->getSavedValue<bool>("macro_hide_recording_label"))
        labelText = "";

    if (labelText == "Playing" && state == state::playing && g.mod->getSavedValue<bool>("macro_hide_playing_label"))
        labelText = "";

    if (g.renderer.recording && g.mod->getSavedValue<bool>("render_hide_labels")) {
        labelText = "";
        if (CCLabelBMFont* lbl = typeinfo_cast<CCLabelBMFont*>(pl->getChildByID("frame-label"_spr)))
            lbl->setString("");
    }

    label->setString(labelText.c_str());
}

void Interface::updateButtons() {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    CCNode* menu = pl->getChildByID("button-menu"_spr);
    if (!menu) return;

    auto& g = Global::get();

#ifdef GEODE_IS_WINDOWS
    bool isWindows = true;
#else
    bool isWindows = false;
#endif

    CCNode* disableStepperBtn = menu->getChildByID("disable-stepper-btn");
    CCNode* stepFrameBtn = menu->getChildByID("step-frame-btn");
    CCNode* speedhackBtn = menu->getChildByID("speedhack-btn");

    disableStepperBtn->setPosition(ccp(
        g.mod->getSavedValue<float>("button_off_pos_x"),
        g.mod->getSavedValue<float>("button_off_pos_y")
    ));

    float scale = g.mod->getSavedValue<float>("button_off_scale");

    CCSprite* sprite = getChildOfType<CCSprite>(disableStepperBtn, 0);
    sprite->setScale(scale);
    sprite->setOpacity(static_cast<int>(g.mod->getSavedValue<float>("button_off_opacity") * 255));
    sprite->setAnchorPoint({ 0, 0 });

    cocos2d::CCSize size = sprite->getContentSize();
    disableStepperBtn->setContentSize({ size.width * scale, size.height * scale });

    stepFrameBtn->setPosition(ccp(
        g.mod->getSavedValue<float>("button_advance_frame_pos_x"),
        g.mod->getSavedValue<float>("button_advance_frame_pos_y")
    ));

    scale = g.mod->getSavedValue<float>("button_advance_frame_scale");

    sprite = getChildOfType<CCSprite>(stepFrameBtn, 0);
    sprite->setScale(scale);
    sprite->setOpacity(static_cast<int>(g.mod->getSavedValue<float>("button_advance_frame_opacity") * 255));
    sprite->setAnchorPoint({ 0, 0 });

    size = sprite->getContentSize();
    speedhackBtn->setContentSize({ size.width * scale, size.height * scale });

    speedhackBtn->setPosition(ccp(
        g.mod->getSavedValue<float>("button_speedhack_pos_x"),
        g.mod->getSavedValue<float>("button_speedhack_pos_y")
    ));

    scale = g.mod->getSavedValue<float>("button_speedhack_scale");

    sprite = getChildOfType<CCSprite>(speedhackBtn, 0);
    sprite->setScale(scale);
    sprite->setOpacity(static_cast<int>(g.mod->getSavedValue<float>("button_speedhack_opacity") * 255));
    sprite->setAnchorPoint({ 0, 0 });

    size = sprite->getContentSize();
    speedhackBtn->setContentSize({ size.width * scale, size.height * scale });

    if ((g.state != state::recording && !g.mod->getSavedValue<bool>("macro_always_show_buttons")) || isWindows) {
        disableStepperBtn->setVisible(false);
        stepFrameBtn->setVisible(false);
        speedhackBtn->setVisible(false);

        return;
    }

    speedhackBtn->setVisible(!g.mod->getSavedValue<bool>("macro_hide_speedhack"));

    if (g.mod->getSavedValue<bool>("macro_hide_stepper")) {
        disableStepperBtn->setVisible(false);
        stepFrameBtn->setVisible(false);
    }
    else {
        stepFrameBtn->setVisible(true);
        disableStepperBtn->setVisible(g.frameStepper);
    }
}