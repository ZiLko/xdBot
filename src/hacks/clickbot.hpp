#include "../includes.hpp"

const std::vector<std::string> buttonNames = { "hold_click", "release_click", "hold_left", "release_left", "hold_right", "release_right" };

const std::unordered_map<int, std::string> buttonInts = { {1, "click"}, {2, "left"}, {3, "right"} };

struct ClickSetting {
    bool disabled = false;
    std::filesystem::path path;
    int volume = 100;
    float pitch = 1.f;
};

template<>
struct matjson::Serialize<ClickSetting> {
    static ClickSetting from_json(matjson::Value const& value) {
        ClickSetting setts;

        setts.disabled = value["disabled"].asBool().unwrapOr(true);
        setts.path = std::filesystem::path(value["path"].asString().unwrapOrDefault());
        setts.volume = value["volume"].asInt().unwrapOrDefault();
        setts.pitch = static_cast<float>(value["pitch"].asDouble().unwrapOrDefault());

        return setts;
    }

    static matjson::Value to_json(ClickSetting const& sett) {
        matjson::Value obj;

        obj["disabled"] = sett.disabled;
        obj["path"] = sett.path.string();
        obj["volume"] = sett.volume;
        obj["pitch"] = sett.pitch;

        return obj;
    }
};

class Clickbot {

public:

    FMOD::System* system = nullptr;
    FMOD::Channel* channel = nullptr;
    FMOD::DSP* pitchShifter = nullptr;

    FMOD::Sound* holdClick = nullptr;
    FMOD::Sound* releaseClick = nullptr;
    FMOD::Sound* holdLeft = nullptr;
    FMOD::Sound* releaseLeft = nullptr;
    FMOD::Sound* holdRight = nullptr;
    FMOD::Sound* releaseRight = nullptr;

    static auto& get() {
        static Clickbot instance;
        return instance;
    }

    static FMOD::Sound* getSound(std::string id) {
        auto& c = get();

        if (id == "hold_click")
            return c.holdClick;
        else if (id == "release_click")
            return c.releaseClick;
        else if (id == "hold_left")
            return c.holdLeft;
        else if (id == "release_left")
            return c.releaseLeft;
        else if (id == "hold_right")
            return c.holdRight;
        else if (id == "release_right")
            return c.releaseRight;

        return nullptr;
    }

    static void setSound(std::string id, FMOD::Sound* sound) {
        auto& c = get();

        if (id == "hold_click")
            c.holdClick = sound;
        else if (id == "release_click")
            c.releaseClick = sound;
        else if (id == "hold_left")
            c.holdLeft = sound;
        else if (id == "release_left")
            c.releaseLeft = sound;
        else if (id == "hold_right")
            c.holdRight = sound;
        else if (id == "release_right")
            c.releaseRight = sound;
    }

    static void playSound(std::string id) {
        auto& c = get();
        if (!c.system) return updateSounds();

        auto& g = Global::get();
        matjson::Value data = g.mod->getSavedValue<matjson::Value>(id);
        ClickSetting settings = matjson::Serialize<ClickSetting>::from_json(data);

        if (settings.disabled) return;

        FMOD::Sound* sound = getSound(id);

        if (!sound) return;

        FMOD_RESULT result;

        result = c.system->playSound(sound, nullptr, false, &c.channel);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 2");

        result = c.channel->setVolume((settings.volume / 100.f) * (g.mod->getSavedValue<int64_t>("clickbot_volume") / 100.f));
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 3");

        result = c.channel->setPitch(g.currentPitch);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 4");

        FMOD::DSP* pitchShifter = c.pitchShifter;
        if (!pitchShifter) return updateSounds();

        result = pitchShifter->setParameterFloat(FMOD_DSP_PITCHSHIFT_PITCH, settings.pitch * g.mod->getSavedValue<float>("clickbot_pitch"));
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 6");

        result = c.channel->addDSP(0, pitchShifter);
        if (result != FMOD_OK) return log::debug("Click sound errored. ID: 7");
    }

    static void updateSounds() {
        auto& c = get();
        FMOD_RESULT result;

        if (!c.system) {
            FMODAudioEngine* fmod = FMODAudioEngine::sharedEngine();
            c.system = fmod->m_system;
        }

        if (!c.system) return;

        for (std::string name : buttonNames) {
            matjson::Value data = Global::get().mod->getSavedValue<matjson::Value>(name);
            ClickSetting settings = matjson::Serialize<ClickSetting>::from_json(data);
            if (!std::filesystem::exists(settings.path)) continue;

            FMOD::Sound* sound = getSound(name);
            result = c.system->createSound(settings.path.string().c_str(), FMOD_DEFAULT, nullptr, &sound);
            if (result != FMOD_OK) {
                log::debug("Click sound errored. ID: 1");
                continue;
            }

            setSound(name, sound);
        }

        if (!c.pitchShifter) {
            result = c.system->createDSPByType(FMOD_DSP_TYPE_PITCHSHIFT, &c.pitchShifter);
            if (result != FMOD_OK) return log::debug("Click sound errored. ID: 5");
        }
    }

};