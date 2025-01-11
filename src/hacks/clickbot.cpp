#include "../includes.hpp"
#include "clickbot.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>

$execute { 
    auto & g = Global::get();

    if (!g.mod->setSavedValue("clickbot_defaults5", true)) {
        g.mod->setSavedValue("clickbot_holding_only", true);
        g.mod->setSavedValue("clickbot_playing_only", false);
    }

    if (!g.mod->setSavedValue("clickbot_defaults4", true)) {
        std::filesystem::path dir = g.mod->getResourcesDir();
        ClickSetting setts;

        for (const auto& str : buttonNames) {
            setts.path = dir / fmt::format("default_{}.mp3", str);
            matjson::Value data = matjson::Serialize<ClickSetting>::to_json(setts);
            g.mod->setSavedValue(str, data);
        }

        g.mod->setSavedValue("clickbot_volume", 100);
        g.mod->setSavedValue("clickbot_pitch", 1.f);
    }

    g.clickbotEnabled = g.mod->getSavedValue<bool>("clickbot_enabled");
    g.clickbotOnlyPlaying = g.mod->getSavedValue<bool>("clickbot_playing_only");
    g.clickbotOnlyHolding = g.mod->getSavedValue<bool>("clickbot_holding_only");

    Clickbot::updateSounds();

};

class $modify(GJBaseGameLayer) {
    
    void handleButton(bool hold, int button, bool player2) {
        GJBaseGameLayer::handleButton(hold, button, player2);
        auto& g = Global::get();

        if (!g.clickbotEnabled) return;
        if (button > 3 || (!hold && g.clickbotOnlyHolding)) return;

        PlayLayer* pl = PlayLayer::get();

        if (!pl) return;
        if (g.clickbotOnlyPlaying && g.state != state::playing) return;

        std::string btn = button == 1 ? "click" : (button == 2 ? "left" : "right");
        std::string id = (hold ? "hold_" : "release_") + btn;
        Clickbot::playSound(id);
    }

};