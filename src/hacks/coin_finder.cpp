#include "show_trajectory.hpp"

#include <Geode/modify/PlayLayer.hpp>

class CoinFinderNode : public cocos2d::CCDrawNode {
public:
    static CoinFinderNode* create() {
        CoinFinderNode* ret = new CoinFinderNode();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;

    }
};

cocos2d::CCDrawNode* drawNode() {

    static CoinFinderNode* instance = nullptr;

    if (!instance) {
        instance = CoinFinderNode::create();
        instance->retain();

        cocos2d::_ccBlendFunc  blendFunc;
        blendFunc.src = GL_SRC_ALPHA;
        blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;

        instance->setBlendFunc(blendFunc);
    }

    return instance;
}

class $modify(PlayLayer) {

    struct Fields {
        std::vector<GameObject*> coins;
    };

    void addObject(GameObject * obj) {
        PlayLayer::addObject(obj);

        if (obj->m_objectType == GameObjectType::UserCoin || obj->m_objectType == GameObjectType::SecretCoin)
            m_fields->coins.push_back(obj);

    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        m_objectLayer->addChild(drawNode(), 499);

    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!Global::get().coinFinder) {
            drawNode()->setVisible(false);
            return;
        }

        drawNode()->clear();
        drawNode()->setVisible(true);

        Global::get().safeMode = true;

        for (GameObject* coin : m_fields->coins) {
            cocos2d::ccColor4F color = { 0.85f, 0.85f, 0.85f, 0.75f };

            if (coin->m_objectType == GameObjectType::SecretCoin)
                color = { 0.96f, 1.f, 0.f, 0.75f };

            drawNode()->drawSegment(m_player1->getPosition(), coin->getPosition(), 0.4f, color);
        }
    }
};