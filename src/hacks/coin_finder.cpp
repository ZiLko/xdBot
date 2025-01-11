#include "coin_finder.hpp"

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

        m_objectLayer->addChild(CoinFinder::drawNode(), 499);

    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!Global::get().coinFinder) return;

        CoinFinder::drawNode()->clear();
        CoinFinder::drawNode()->setVisible(true);

        Global::get().safeMode = true;

        for (GameObject* coin : m_fields->coins) {
            cocos2d::ccColor4F color = { 0.85f, 0.85f, 0.85f, 0.75f };

            if (coin->m_objectType == GameObjectType::SecretCoin)
                color = { 0.96f, 1.f, 0.f, 0.75f };

            CoinFinder::drawNode()->drawSegment(m_player1->getPosition(), coin->getPosition(), 0.4f, color);
        }
    }
};