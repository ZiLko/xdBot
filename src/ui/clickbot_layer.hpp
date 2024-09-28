#pragma once

#include "../includes.hpp"
#include "../hacks/clickbot.hpp"
#include "record_layer.hpp"

const std::unordered_map<int, std::string> buttons = { {1, ""} };

class ClickSettingsLayer : public geode::Popup<std::string, geode::Popup<>*> {

private:

    ClickSetting settings;
    std::string button;

    Slider* volumeSlider = nullptr;
    Slider* pitchSlider = nullptr;

    CCLabelBMFont* filenameLabel = nullptr;
    CCLabelBMFont* volumeLabel = nullptr;
    CCLabelBMFont* pitchLabel = nullptr;

    bool setup(std::string button, geode::Popup<>* layer) override;

public:

    geode::Popup<>* clickbotLayer = nullptr;

    static ClickSettingsLayer* create(std::string button, geode::Popup<>* layer);

    void saveSettings() {
        matjson::Value data = matjson::Serialize<ClickSetting>::to_json(settings);
        Mod::get()->setSavedValue(button, data);
    }

    void onSelectFile(CCObject*);

    void onDisable(CCObject* obj) {
        CCMenuItemToggler* toggle = static_cast<CCMenuItemToggler*>(obj);

        settings.disabled = !toggle->isToggled();

        saveSettings();
    }

    void updateVolume(CCObject*) {
        settings.volume = static_cast<int>(volumeSlider->getThumb()->getValue() * 300.f);

        volumeLabel->setString(("Volume (" + std::to_string(settings.volume) + "%)").c_str());

        saveSettings();
    }

    void updatePitch(CCObject*) {
        settings.pitch = pitchSlider->getThumb()->getValue() * 3.f;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << settings.pitch;

        if (oss.str() == "1.0")
            settings.pitch = 1.f;

        pitchLabel->setString(("Pitch (" + oss.str() + ")").c_str());

        saveSettings();
    }

};


class ClickbotLayer : public geode::Popup<> {

    Slider* volumeSlider = nullptr;
    Slider* pitchSlider = nullptr;

    CCLabelBMFont* volumeLabel = nullptr;
    CCLabelBMFont* pitchLabel = nullptr;

private:

    bool setup() override;

public:

    STATIC_CREATE(ClickbotLayer, 432, 250)
    
    std::vector<CCLabelBMFont*> labels;

    void open(CCObject*) {
        ClickbotLayer::create()->show();
    }

    void openClickSettings(CCObject* obj) {
        std::string id = static_cast<CCMenuItemSpriteExtra*>(obj)->getID();
        ClickSettingsLayer::create(id, static_cast<geode::Popup<>*>(this))->show();
    }

    void updateLabels();

    void updateVolume(CCObject*) {
        int volume = static_cast<int>(volumeSlider->getThumb()->getValue() * 300.f);

        volumeLabel->setString(("Master Volume (" + std::to_string(volume) + "%)").c_str());

        Mod::get()->setSavedValue("clickbot_volume", volume);
    }

    void updatePitch(CCObject*) {
        float pitch = pitchSlider->getThumb()->getValue() * 3.f;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << pitch;

        if (oss.str() == "1.0")
            pitch = 1.f;

        pitchLabel->setString(("Master Pitch (" + oss.str() + ")").c_str());

        Mod::get()->setSavedValue("clickbot_pitch", pitch);
    }


};