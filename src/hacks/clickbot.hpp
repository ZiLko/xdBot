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

    static FMOD::Sound* getSound(std::string id);

    static void setSound(std::string, FMOD::Sound*);

    static void playSound(std::string);

    static void updateSounds();

};