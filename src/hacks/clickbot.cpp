#include "../includes.hpp"
#include "clickbot.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>

class $modify(GJBaseGameLayer) {
    
    struct Fields {
        int lastClickFrame = 0;
    };

    void handleButton(bool hold, int button, bool player2) {
        GJBaseGameLayer::handleButton(hold, button, player2);
        auto& g = Global::get();

        if (!g.clickbotEnabled) return;
        if (button > 3) return;

        PlayLayer* pl = PlayLayer::get();

        if (!pl) return;
        if (g.mod->getSavedValue<bool>("clickbot_playing_only") && g.state != state::playing) return;

        std::string buttonName = (hold ? "hold_" : "release_") + buttonInts.at(button);

        matjson::Value data = g.mod->getSavedValue<matjson::Value>(buttonName);
        ClickSetting settings = matjson::Serialize<ClickSetting>::from_json(data);

        if (settings.disabled) return;
        if (!std::filesystem::exists(settings.path)) return;

        int frame = Global::getCurrentFrame();
        if (m_fields->lastClickFrame == frame) return;
        m_fields->lastClickFrame = frame;

        FMODAudioEngine* fmod = FMODAudioEngine::sharedEngine();
        FMOD::System* system = fmod->m_system;

        FMOD::Channel* channel = nullptr;
        FMOD::Sound* sound = nullptr;
        FMOD::DSP* pitchShifter = nullptr;

        FMOD_RESULT result;

        result = system->createSound(settings.path.string().c_str(), FMOD_DEFAULT, nullptr, &sound);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 1");

        result = system->playSound(sound, nullptr, false, &channel);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 2");

        result = channel->setVolume((settings.volume / 100.f) * (g.mod->getSavedValue<int64_t>("clickbot_volume") / 100.f));
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 3");

        result = channel->setPitch(g.currentPitch);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 4");

        result = system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &pitchShifter);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 5");

        result = pitchShifter->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, settings.pitch * g.mod->getSavedValue<float>("clickbot_pitch"));
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 6");

        result = channel->addDSP(0, pitchShifter);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 7");
    }
};