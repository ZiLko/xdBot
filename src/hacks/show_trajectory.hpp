#include "../includes.hpp"
#include "../practice_fixes/practice_fixes.hpp"
#include "layout_mode.hpp"

const std::unordered_set<int> portalIDs = { 101, 99, 11, 10, 200, 201, 202, 203, 1334 };
const std::unordered_set<int> collectibleIDs = { 1329,1275,1587,1589,1598,1614,3601,4401,4402,4403,4404,4405,4406,4407,4408,4409,4410,4411,4412,4413,4414,4415,4416,4417,4418,4419,4420,4421,4422,4423,4424,4425,4426,4427,4428,4429,4430,4431,4432,4433,4434,4435,4436,4437,4438,4439,4440,4441,4442,4443,4444,4445,4446,4447,4448,4449,4450,4451,4452,4453,4454,4455,4456,4457,4458,4459,4460,4461,4462,4463,4464,4465,4466,4467,4468,4469,4470,4471,4472,4473,4474,4475,4476,4477,4478,4479,4480,4481,4482,4483,4484,4485,4486,4487,4488,4538,4489,4490,4491,4492,4493,4494,4495,4496,4497,4537,4498,4499,4500,4501,4502,4503,4504,4505,4506,4507,4508,4509,4510,4511,4512,4513,4514,4515,4516,4517,4518,4519,4520,4521,4522,4523,4524,4525,4526,4527,4528,4529,4530,4531,4532,4533,4534,4535,4536,4539 };
const std::unordered_set<int> objectTypes = { 0, 2, 47, 25 };

class TrajectoryNode : public cocos2d::CCDrawNode {
public:
    static TrajectoryNode* create() {
        TrajectoryNode* ret = new TrajectoryNode();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;

    }
};

class ShowTrajectory {

public:

    static cocos2d::CCDrawNode* trajectoryNode();

    static void updateTrajectory(PlayLayer* pl);

    static CCDrawNode* createNode();

    static void createTrajectory(PlayLayer* pl, PlayerObject* fakePlayer, PlayerObject* realPlayer, bool hold);

    static void drawPlayerHitbox(PlayerObject* player, CCDrawNode* drawNode);

    static std::vector<cocos2d::CCPoint> getVertices(PlayerObject* player, cocos2d::CCRect rect, float rotation);

    static cocos2d::ccColor4F getMergedColor(cocos2d::ccColor4F color1, cocos2d::ccColor4F color2);

    static void handlePortal(PlayerObject* player, int id);

    PlayerObject* fakePlayer1 = nullptr;
    PlayerObject* fakePlayer2 = nullptr;

    bool creatingTrajectory = false;
    bool cancelTrajectory = false;
    bool miniScale = false;

    float deathRotation = 0.f;
    float delta = 0.25f;

    int length = 312;

    cocos2d::ccColor4F color1;
    cocos2d::ccColor4F color2;
    cocos2d::ccColor4F color3;

    cocos2d::CCPoint player1Trajectory[480];
    cocos2d::CCPoint player2Trajectory[480];

};