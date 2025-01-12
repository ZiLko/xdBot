#include <Geode/Geode.hpp>

#include "gdr.hpp"

cocos2d::CCPoint dataFromString(std::string dataString) {
    std::stringstream ss(dataString);
    std::string item;
    std::vector<std::string> vec;

    float xPos = 0.f;
    float yPos = 0.f;

    for (int i = 0; i < 3; i++) {
        std::getline(ss, item, ',');
        if (i == 1)
            xPos = std::stof(item);
        else if (i == 2)
            yPos = std::stof(item);
    }

    return { xPos, yPos };
};

std::vector<std::string> splitByChar(std::string str, char splitChar) {
    std::vector<std::string> strs;
    strs.reserve(std::count(str.begin(), str.end(), splitChar) + 1);

    size_t start = 0;
    size_t end = str.find(splitChar);
    while (end != std::string::npos) {
        strs.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(splitChar, start);
    }
    strs.emplace_back(str.substr(start));

    return strs;
}

geode::prelude::VersionInfo getVersion(std::vector<std::string> nums) {
    size_t major = geode::utils::numFromString<int>(nums[0]).unwrapOr(-1);
    size_t minor = geode::utils::numFromString<int>(nums[1]).unwrapOr(-1);
    size_t patch = geode::utils::numFromString<int>(nums[2]).unwrapOr(-1);
    
    geode::prelude::VersionInfo ret(major, minor, patch);

    return ret;
}
