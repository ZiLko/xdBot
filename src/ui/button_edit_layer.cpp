#include "button_edit_layer.hpp"
#include "game_ui.hpp"

#include <Geode/modify/FLAlertLayer.hpp>

$execute{
    auto & g = Global::get();

  if (!g.mod->setSavedValue("button_defaults5", true)) {
    cocos2d::CCSize winSize = CCDirector::sharedDirector()->getWinSize();

    g.mod->setSavedValue("button_off_pos_x", 62.f);
    g.mod->setSavedValue("button_off_pos_y", winSize.height - 35.f);
    g.mod->setSavedValue("button_off_scale", 1.f);
    g.mod->setSavedValue("button_off_opacity", 1.f);

    g.mod->setSavedValue("button_advance_frame_pos_x", 100.f);
    g.mod->setSavedValue("button_advance_frame_pos_y", winSize.height - 50.f);
    g.mod->setSavedValue("button_advance_frame_scale", 0.9f);
    g.mod->setSavedValue("button_advance_frame_opacity", 1.f);

    g.mod->setSavedValue("button_speedhack_pos_x", winSize.width - 62.f);
    g.mod->setSavedValue("button_speedhack_pos_y", winSize.height - 38.f);
    g.mod->setSavedValue("button_speedhack_scale", 1.f);
    g.mod->setSavedValue("button_speedhack_opacity", 1.f);
  }
}

class $modify(FLAlertLayer) {

    virtual bool ccTouchBegan(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        if (!FLAlertLayer::ccTouchBegan(touch, event)) return false;
        ButtonEditLayer* layer = typeinfo_cast<ButtonEditLayer*>(this);

        if (!layer) return true;

        if (layer->movingButton.sprite) return true;

        std::string selected = "";

        // Latest has priority
        for (size_t i = 0; i < layer->spriteButtons.size(); i++) {
            cocos2d::CCPoint location = touch->getLocation();
            cocos2d::CCPoint btnPos = layer->spriteButtons[i]->getPosition();
            cocos2d::CCSize btnSize = layer->spriteButtons[i]->getContentSize() * layer->spriteButtons[i]->getScale();

            if (ButtonEditLayer::isPointInButton(location, btnPos, btnSize)) {
                layer->movingButton = { i, layer->spriteButtons[i], location - btnPos };
                selected = indexToID[i];
            }
        }

        if (selected != "")
            layer->updateSelected(selected);

        return true;
    }

    virtual void ccTouchMoved(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        FLAlertLayer::ccTouchMoved(touch, event);
        ButtonEditLayer* layer = typeinfo_cast<ButtonEditLayer*>(this);

        if (!layer) return;

        if (!layer->movingButton.sprite) return;

        cocos2d::CCPoint position = touch->getLocation() - layer->movingButton.offset;

        layer->movingButton.sprite->setPosition(position);

        layer->positions[indexToID[layer->movingButton.index]] = position;
    }

    virtual void ccTouchEnded(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        FLAlertLayer::ccTouchEnded(touch, event);
        ButtonEditLayer* layer = typeinfo_cast<ButtonEditLayer*>(this);

        if (!layer) return;

        layer->movingButton.sprite = nullptr;
    }

};

void ButtonEditLayer::onSave(CCObject*) {
    for (const std::string& id : indexToID) {
        mod->setSavedValue(id + "_pos_x", positions.at(id).x);
        mod->setSavedValue(id + "_pos_y", positions.at(id).y);
        mod->setSavedValue(id + "_scale", scales.at(id));
        mod->setSavedValue(id + "_opacity", opacities.at(id));
    }

    onClose(nullptr);
    Interface::updateButtons();
}

bool ButtonEditLayer::isPointInButton(cocos2d::CCPoint clickPos, cocos2d::CCPoint pos, cocos2d::CCSize size) {
    bool xCoincides;
    bool yCoincides;

    xCoincides = clickPos.x >= pos.x && clickPos.x <= pos.x + size.width;
    yCoincides = clickPos.y >= pos.y && clickPos.y <= pos.y + size.height;

    return xCoincides && yCoincides;
}

void ButtonEditLayer::updateSelectedLabels() {
    std::string selected = indexToID[currentSelected];

    float scale = scales.at(selected);
    float opacity = opacities.at(selected);

    std::ostringstream oss;

    oss << std::fixed << std::setprecision(1) << scale;

    scaleLbl->setString(std::format("Scale ({})", oss.str()).c_str());

    std::ostringstream oss2;
    oss2 << std::fixed << std::setprecision(1) << opacity;

    opacityLbl->setString(std::format("Opacity ({})", oss2.str()).c_str());
}

void ButtonEditLayer::updateSelected(std::string selected) {
    currentSelected = IDtoIndex.at(selected);

    updateSelectedLabels();

    if (scaleSlider)
        scaleSlider->removeFromParentAndCleanup(true);

    if (opacitySlider)
        opacitySlider->removeFromParentAndCleanup(true);

    addSliders();

    selectedLbl->setString(IDtoName.at(selected).c_str());
}

void ButtonEditLayer::updateScale(CCObject*) {
    float value = scaleSlider->getThumb()->getValue();

    if (value < 0.12f) {
        value = 0.12f;
        scaleSlider->getThumb()->setValue(value);
    }

    spriteButtons[currentSelected]->setScale(value);

    scales[indexToID[currentSelected]] = value;

    updateSelectedLabels();
}

void ButtonEditLayer::updateOpacity(CCObject*) {
    float value = opacitySlider->getThumb()->getValue();

    if (value < 0.05f) {
        value = 0.05f;
        opacitySlider->getThumb()->setValue(value);
    }

    spriteButtons[currentSelected]->setOpacity(static_cast<int>(value * 255));

    opacities[indexToID[currentSelected]] = value;

    updateSelectedLabels();
}

bool ButtonEditLayer::setup() {
    mod = Mod::get();

    cocos2d::CCPoint offset = (CCDirector::sharedDirector()->getWinSize() - m_mainLayer->getContentSize()) / 2;
    m_mainLayer->setPosition(m_mainLayer->getPosition() - offset);
    m_closeBtn->setPosition(m_closeBtn->getPosition() + offset);
    m_bgSprite->setPosition(m_bgSprite->getPosition() + offset);

    Utils::setBackgroundColor(m_bgSprite); 

    m_bgSprite->setPositionY(231);
    m_closeBtn->setVisible(false);
    setOpacity(207);

    menu = CCMenu::create();
    m_mainLayer->addChild(menu);

    selectedLbl = CCLabelBMFont::create("Frame Stepper Off", "goldFont.fnt");
    selectedLbl->setScale(0.5f);
    selectedLbl->setPositionY(121);
    menu->addChild(selectedLbl);

    CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0,0,0 });
    bg->setOpacity(78);
    bg->setPositionY(77);
    bg->setAnchorPoint({ 0.5, 0.5 });
    bg->setContentSize({ 180, 60 });
    menu->addChild(bg);

    scaleLbl = CCLabelBMFont::create("Scale (1.0)", "chatFont.fnt");
    scaleLbl->setScale(0.625f);
    scaleLbl->setPosition({ -79, 89 });
    scaleLbl->setAnchorPoint({ 0, 0.5 });
    menu->addChild(scaleLbl);

    opacityLbl = CCLabelBMFont::create("Opacity (1.0)", "chatFont.fnt");
    opacityLbl->setScale(0.625f);
    opacityLbl->setPosition({ -79, 66 });
    opacityLbl->setAnchorPoint({ 0, 0.5 });
    menu->addChild(opacityLbl);

    ButtonSprite* spr = ButtonSprite::create("Save");
    spr->setScale(0.6f);

    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ButtonEditLayer::onSave));
    btn->setPosition({ 48, 30 });
    menu->addChild(btn);

    spr = ButtonSprite::create("Cancel");
    spr->setScale(0.6f);

    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ButtonEditLayer::onClose));
    btn->setPosition({ -40, 30 });
    menu->addChild(btn);

    addSprites();

    for (size_t i = 0; i < spriteButtons.size(); i++) {
        cocos2d::CCPoint position = ccp(
            mod->getSavedValue<float>(indexToID[i] + "_pos_x"),
            mod->getSavedValue<float>(indexToID[i] + "_pos_y")
        );
        float scale = mod->getSavedValue<float>(indexToID[i] + "_scale");
        float opacity = mod->getSavedValue<float>(indexToID[i] + "_opacity");

        positions[indexToID[i]] = position;
        scales[indexToID[i]] = scale;
        opacities[indexToID[i]] = opacity;
    }

    addSliders();
    updateScale(nullptr);
    updateScale(nullptr);
    updateSelected("button_advance_frame");

    return true;
}

void ButtonEditLayer::addSliders() {
    opacitySlider = Slider::create(
        this,
        menu_selector(ButtonEditLayer::updateOpacity),
        0.8f
    );
    opacitySlider->setPosition({ 31, 66 });
    opacitySlider->setAnchorPoint({ 0.f, 0.f });
    opacitySlider->setScale(0.5f);
    opacitySlider->setValue(opacities[indexToID[currentSelected]]);
    menu->addChild(opacitySlider);

    scaleSlider = Slider::create(
        this,
        menu_selector(ButtonEditLayer::updateScale),
        0.8f
    );
    scaleSlider->setPosition({ 31, 89 });
    scaleSlider->setAnchorPoint({ 0.f, 0.f });
    scaleSlider->setScale(0.5f);
    scaleSlider->setValue(scales[indexToID[currentSelected]]);
    menu->addChild(scaleSlider);
}

void ButtonEditLayer::addSprites() {
    CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png");
    spr->setAnchorPoint({ 0, 0 });

    cocos2d::CCPoint pos = ccp(
        mod->getSavedValue<float>("button_off_pos_x"),
        mod->getSavedValue<float>("button_off_pos_y")
    );

    spr->setPosition(pos);
    spr->setScale(mod->getSavedValue<float>("button_off_scale"));
    spr->setOpacity(static_cast<int>(mod->getSavedValue<float>("button_off_opacity") * 255));

    m_mainLayer->addChild(spr);
    spriteButtons.push_back(spr);

    spr = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    spr->setAnchorPoint({ 0, 0 });
    spr->setFlipX(true);

    pos = ccp(
        mod->getSavedValue<float>("button_advance_frame_pos_x"),
        mod->getSavedValue<float>("button_advance_frame_pos_y")
    );

    spr->setPosition(pos);
    spr->setScale(mod->getSavedValue<float>("button_advance_frame_scale"));
    spr->setOpacity(static_cast<int>(mod->getSavedValue<float>("button_advance_frame_opacity") * 255));

    m_mainLayer->addChild(spr);
    spriteButtons.push_back(spr);

    spr = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    spr->setAnchorPoint({ 0, 0 });

    pos = ccp(
        mod->getSavedValue<float>("button_speedhack_pos_x"),
        mod->getSavedValue<float>("button_speedhack_pos_y")
    );

    spr->setPosition(pos);
    spr->setScale(mod->getSavedValue<float>("button_speedhack_scale"));
    spr->setOpacity(static_cast<int>(mod->getSavedValue<float>("button_speedhack_opacity") * 255));

    m_mainLayer->addChild(spr);
    spriteButtons.push_back(spr);
}