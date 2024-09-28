#include "../includes.hpp"

class InputPracticeFixes {
public:
    static void applyFixes(PlayLayer* pl, PlayerData p1Data, PlayerData p2Data, int frame);

    static void eraseActions(int frame);

    static std::vector<button> findButtons();

    static std::vector<int> fixInputs(std::vector<button> foundButtons, PlayLayer* pl, PlayerData p1Data, PlayerData p2Data, int frame);

};

class PlayerPracticeFixes {
public:

    static void applyData(PlayerObject* player, PlayerData data, PlayerObject* player2, bool isFakePlayer = false);

    static PlayerData saveData(PlayerObject* player);

};