// #pragma once

// #include "../includes.hpp"

// // #include <Geode/loader/SettingNode.hpp>

// class ButtonSettingValue : public SettingValue {

// public:

//     ButtonSettingValue(std::string const& key, std::string const& modID, std::string const& placeholder)
//         : SettingValue(key, modID) {}

//     bool load(matjson::Value const& json) override { return true; }

//     bool save(matjson::Value& json) const override { return true; }

//     SettingNode* createNode(float width) override;

// };

// class ButtonSettingNode : public SettingNode {

// protected:

//     bool init(ButtonSettingValue* value, float width) {
//         if (!SettingNode::init(value)) return false;

//         setContentSize({ width, 35.f });

//         CCMenu* menu = CCMenu::create();
//         menu->setPosition({ 0, 0 });
//         addChild(menu);

//         CCLabelBMFont* lbl = CCLabelBMFont::create("Open Menu", "bigFont.fnt");
//         lbl->setScale(0.525f);
//         lbl->setAnchorPoint({ 0, 0.5 });
//         lbl->setPosition({ 20, 18 });
//         menu->addChild(lbl);

//         CCSprite* sprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
//         sprite->setScale(0.34125f);

//         CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
//             sprite,
//             this,
//             menu_selector(ButtonSettingNode::onOpen)
//         );
//         btn->setPosition({ 260, 18 });
//         menu->addChild(btn);

//         return true;
//     }

// public:

//     void onOpen(CCObject*) {
//         CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
//         if (FLAlertLayer* layer = typeinfo_cast<FLAlertLayer*>(children->lastObject()))
//             layer->keyBackClicked();

//         RecordLayer::openMenu();
//     }

//     void commit() override { this->dispatchCommitted(); }

//     bool hasUncommittedChanges() override { return false; }

//     bool hasNonDefaultValue() override { return true; }

//     void resetToDefault() override { }

//     static ButtonSettingNode* create(ButtonSettingValue* value, float width) {
//         ButtonSettingNode* ret = new ButtonSettingNode();
//         if (ret->init(value, width)) {
//             ret->autorelease();
//             return ret;
//         }

//         delete ret;
//         return nullptr;
//     }
// };
