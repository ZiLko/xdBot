#pragma once
#include "../includes.hpp"

class SaveMacroLayer : public geode::Popup<> {

    TextInput* authorInput = nullptr;
    TextInput* descInput = nullptr;
    TextInput* nameInput = nullptr;

    CCMenuItemToggler* jsonToggle = nullptr;

private:

    bool setup() override {
        Utils::setBackgroundColor(m_bgSprite);

        setTitle("Save Macro");

        authorInput = TextInput::create(104, "Author", "chatFont.fnt");
        authorInput->setPosition({ 61, 42 });
        authorInput->setString(GJAccountManager::sharedState()->m_username.c_str());
        m_buttonMenu->addChild(authorInput);

        CCLabelBMFont* lbl = CCLabelBMFont::create("(optional)", "chatFont.fnt");
        lbl->setPosition({ 61, 20 });
        lbl->setOpacity(73);
        lbl->setScale(0.575);
        m_buttonMenu->addChild(lbl);

        nameInput = TextInput::create(104, "Name", "chatFont.fnt");
        nameInput->setPosition({ -61, 42 });

        nameInput->setString(Global::get().macro.levelInfo.name);

        m_buttonMenu->addChild(nameInput);

        descInput = TextInput::create(226, "Description (optional)", "chatFont.fnt");
        descInput->setPositionY(-8);
        m_buttonMenu->addChild(descInput);

        ButtonSprite* spr = ButtonSprite::create("Save");
        spr->setScale(0.725f);
        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(SaveMacroLayer::onSave));
        btn->setPositionY(-56);
        m_buttonMenu->addChild(btn);

        // CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        // CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
        // jsonToggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, nullptr);
        // jsonToggle->setPosition({ -124, -78 });
        // jsonToggle->setScale(0.575);
        // m_buttonMenu->addChild(jsonToggle);

        // lbl = CCLabelBMFont::create("JSON", "bigFont.fnt");
        // lbl->setPosition({ -97, -77.5 });
        // lbl->setScale(0.375);
        // m_buttonMenu->addChild(lbl);

        return true;
    }

public:

    STATIC_CREATE(SaveMacroLayer, 285, 194)
    
    static void open() {
        if (Global::get().macro.inputs.empty())
            return FLAlertLayer::create("Save Macro", "You can't save an <cl>empty</c> macro.", "Ok")->show();

        std::filesystem::path path = Mod::get()->getSaveDir() / "macros";

        if (!std::filesystem::exists(path)) {
            if (!std::filesystem::create_directory(path))
                return FLAlertLayer::create("Error", "There was an error getting the folder. ID: 10", "Ok")->show();
        }

        SaveMacroLayer* layerReal = create();
        layerReal->m_noElasticity = true;
        layerReal->show();
    }

    void onSave(CCObject*) {
        std::string macroName = nameInput->getString();
        if (macroName == "")
            return FLAlertLayer::create("Save Macro", "Give a <cl>name</c> to the macro.", "Ok")->show();

        std::filesystem::path path = Mod::get()->getSaveDir() / "macros" / macroName;
        std::string author = authorInput->getString();
        std::string desc = descInput->getString();

        int result = Macro::save(author, desc, path.string(), true); // Previously jsonToggle->isToggled()

        if (result != 0)
            return FLAlertLayer::create("Error", "There was an error saving the macro. ID: " + std::to_string(result), "Ok")->show();

        this->keyBackClicked();
        Notification::create("Macro Saved", NotificationIcon::Success)->show();
    }

};