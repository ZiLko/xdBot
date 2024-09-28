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