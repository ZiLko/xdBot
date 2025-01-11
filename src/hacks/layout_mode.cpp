#include "layout_mode.hpp"

#include <Geode/modify/LevelTools.hpp>
#include <Geode/modify/PlayLayer.hpp>

class $modify(LevelTools) {
    static bool verifyLevelIntegrity(gd::string v1, int v2) {
        if (Global::get().layoutMode) return true;
        return LevelTools::verifyLevelIntegrity(v1, v2);
    }
};

class $modify(PlayLayer) {

    void addObject(GameObject * obj) {
        if (!Global::get().layoutMode) return PlayLayer::addObject(obj);

        if (excludedTriggerIDs.contains(obj->m_objectID)) return; 

        PlayLayer::addObject(obj);

        obj->m_activeMainColorID = -1;
        obj->m_activeDetailColorID = -1;
        obj->m_detailUsesHSV = false;
        obj->m_baseUsesHSV = false;
        obj->m_hasNoGlow = true;
        obj->m_isHide = obj->m_objectID == 2065;
        obj->setOpacity(obj->m_objectID == 2065 ? 0 : 255);
        obj->setVisible(obj->m_objectID != 2065);
    }

    bool init(GJGameLevel * level, bool b1, bool b2) {
        auto& g = Global::get();

#ifdef GEODE_IS_ANDROID

        if (robtopLevelIDs.contains(level->m_levelID.value()))
            return PlayLayer::init(level, b1, b2);

#endif

        g.layoutMode = Mod::get()->getSavedValue<bool>("macro_layout_mode");
        std::string oldString = level->m_levelString;

        if (g.layoutMode)
            level->m_levelString = LayoutMode::getModifiedString(level->m_levelString);

        if (!PlayLayer::init(level, b1, b2)) {
            if (g.layoutMode)
                level->m_levelString = oldString;

            return false;
        }

        if (g.layoutMode)
            level->m_levelString = oldString;

        return true;
    }
};

std::string LayoutMode::getModifiedString(std::string levelString) {
    if (levelString.empty()) return "";

    std::string decompString = ZipUtils::decompressString(levelString.c_str(), true, 0);

    std::vector<std::string> objectStrings = Utils::splitByChar(decompString, ';');
    std::string firstPart = objectStrings[0];

    std::vector<std::string> levelSettings = Utils::splitByChar(firstPart, ',');
    std::vector<std::string> levelColors = Utils::splitByChar(levelSettings[1], '|');

    if (levelColors.size() >= 6)
        levelColors.erase(levelColors.begin(), levelColors.begin() + 6);

    levelSettings[1] = newColors;

    firstPart = mergeVector(levelSettings);

    std::unordered_set<int> importantGroups;

    for (int i = 0; i < levelSettings.size(); i++) {
        if (levelSettings[i] == "kA36") {
            if (std::stoi(levelSettings[i + 1]) != 0)
                importantGroups.insert(std::stoi(levelSettings[i + 1]));

            break;
        }
    }

    std::vector<LevelObject> objects;

    for (int i = 1; i < objectStrings.size(); i++) {
        std::vector<std::string> result = Utils::splitByChar(objectStrings[i], ',');
        std::map<int, std::string> props;
        size_t hidden = 0;

        for (size_t i = 0; i <= result.size(); i += 2) {
            if (i >= result.size()) break;
            int propID;

            try {
                propID = std::stoi(result[i]);
            }
            catch (const std::invalid_argument& e) {
                continue;
            }
            catch (const std::out_of_range& e) {
                continue;
            }

            if (result[i] == "135") hidden = i;
            props[std::stoi(result[i])] = result[i + 1];
        }

        objects.push_back({ result, props, hidden, objectStrings[i] });

        if (!props.contains(1)) continue;

        if (!importantTriggerIDs.contains(std::stoi(props.at(1)))) continue;

        for (int i = 0; i < importantTriggerIDs.at(std::stoi(props.at(1))).size(); i++) {
            int propID = importantTriggerIDs.at(std::stoi(props.at(1)))[i];
            if (!props.contains(propID)) continue;

            if (std::stoi(props[propID]) != 0)
                importantGroups.insert(std::stoi(props[propID]));
        }

    }

    std::string newString = "";

    for (LevelObject obj : objects) {

        std::vector<std::string> vec = obj.vecString;
        std::map<int, std::string> props = obj.props;
        int hidden = obj.hiddenIndex;

        if (!props.contains(1)) continue;

        if (decoObjectIDs.contains(std::stoi(props.at(1))) || props.contains(121)) {
            if (!props.contains(57)) continue;

            for (const auto& el : Utils::splitByChar(props.at(57), '.')) {
                if (el.empty()) continue;
                if (!importantGroups.contains(std::stoi(el))) continue;

                if (!props.contains(135)) {
                    vec.push_back("135");
                    vec.push_back("1");
                }
                newString += ";" + mergeVector(vec);

                break;
            }

            continue;
        }

        if (!solidObjectIDs.contains(std::stoi(props.at(1)))) {
            newString += ";" + obj.ogString;
            continue;
        }

        if (props.contains(129)) {
            if (std::stof(props[129]) <= 0.f && hidden == 0) {
                vec.push_back("135");
                vec.push_back("1");

                newString += ";" + mergeVector(vec);
                continue;
            }
        }

        if (hidden != 0) {
            vec.erase(vec.begin() + hidden);
            vec.erase(vec.begin() + hidden);
            newString += ";" + mergeVector(vec);
        }
        else
            newString += ";" + obj.ogString;
    }

    return firstPart + newString + ";";

}

std::string LayoutMode::mergeVector(std::vector<std::string> vec, std::string separator) {
    if (vec.empty()) return "";

    std::string result;

    size_t total_size = 0;
    for (const auto& s : vec)
        total_size += s.size() + separator.size();

    result.reserve(total_size);

    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];

        if (i != vec.size() - 1)
            result += separator;
    }

    return result;
}
