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

        setts.disabled = value["disabled"].as_bool();
        setts.path = std::filesystem::path(value["path"].as_string());
        setts.volume = value["volume"].as_int();
        setts.pitch = static_cast<float>(value["pitch"].as_double());

        return setts;
    }

    static matjson::Value to_json(ClickSetting const& sett) {
        auto obj = matjson::Object();

        obj["disabled"] = sett.disabled;
        obj["path"] = sett.path.string();
        obj["volume"] = sett.volume;
        obj["pitch"] = sett.pitch;

        return obj;
    }
};

$execute{
    auto & g = Global::get();

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
        g.mod->setSavedValue("clickbot_playing_only", true);
        g.mod->setSavedValue("clickbot_enabled", false);

    }

    g.clickbotEnabled = g.mod->getSavedValue<bool>("clickbot_enabled");

};