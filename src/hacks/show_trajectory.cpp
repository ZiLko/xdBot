#include "show_trajectory.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

ShowTrajectory& t = ShowTrajectory::get();

$execute {

    t.color1 = ccc4FFromccc3B(Mod::get()->getSavedValue<cocos2d::ccColor3B>("trajectory_color1"));
    t.color2 = ccc4FFromccc3B(Mod::get()->getSavedValue<cocos2d::ccColor3B>("trajectory_color2"));
    t.length = geode::utils::numFromString<int>(Mod::get()->getSavedValue<std::string>("trajectory_length")).unwrapOr(0);
    t.updateMergedColor();

};

void ShowTrajectory::trajectoryOff() {
    if (t.trajectoryNode()) {
        t.trajectoryNode()->clear();
        t.trajectoryNode()->setVisible(false);
    }
}

void ShowTrajectory::updateTrajectory(PlayLayer* pl) {
    if (!t.fakePlayer1 || !t.fakePlayer2) return;

    auto& g = Global::get();

    g.safeMode = true;

    t.creatingTrajectory = true;
    g.creatingTrajectory = true;

    t.trajectoryNode()->setVisible(true);
    t.trajectoryNode()->clear();

    if (t.fakePlayer1 && pl->m_player1) {
        createTrajectory(pl, t.fakePlayer1, pl->m_player1, true);
        createTrajectory(pl, t.fakePlayer2, pl->m_player1, false);

        if (g.trajectoryBothSides) {
            createTrajectory(pl, t.fakePlayer1, pl->m_player1, true, true);
            createTrajectory(pl, t.fakePlayer2, pl->m_player1, false, true);
        }
    }

    if (pl->m_gameState.m_isDualMode && pl->m_player2) {
        createTrajectory(pl, t.fakePlayer2, pl->m_player2, true);
        createTrajectory(pl, t.fakePlayer1, pl->m_player2, false);

        if (g.trajectoryBothSides) {
            createTrajectory(pl, t.fakePlayer2, pl->m_player2, true, true);
            createTrajectory(pl, t.fakePlayer1, pl->m_player2, false, true);
        }
    }

    t.creatingTrajectory = false;
    g.creatingTrajectory = false;
}
float rot = 0.f;
void ShowTrajectory::createTrajectory(PlayLayer* pl, PlayerObject* fakePlayer, PlayerObject* realPlayer, bool hold, bool inverted) {

    bool player2 = pl->m_player2 == realPlayer;

    PlayerData playerData = PlayerPracticeFixes::saveData(realPlayer);
    PlayerPracticeFixes::applyData(fakePlayer, playerData, false, true);

    t.cancelTrajectory = false;

    for (int i = 0; i < t.length; i++) {
        CCPoint prevPos = fakePlayer->getPosition();

        if (hold) {
            if (player2)
                t.player2Trajectory[i] = prevPos;
            else
                t.player1Trajectory[i] = prevPos;
        }

        fakePlayer->m_collisionLogTop->removeAllObjects();
        fakePlayer->m_collisionLogBottom->removeAllObjects();
        fakePlayer->m_collisionLogLeft->removeAllObjects();
        fakePlayer->m_collisionLogRight->removeAllObjects();

        pl->checkCollisions(fakePlayer, t.delta, false);

        if (t.cancelTrajectory) {
            fakePlayer->updatePlayerScale();
            drawPlayerHitbox(fakePlayer, t.trajectoryNode());
            break;
        }

        if (i == 0) {
            hold ? fakePlayer->pushButton(static_cast<PlayerButton>(1)) : fakePlayer->releaseButton(static_cast<PlayerButton>(1));
            if (pl->m_levelSettings->m_platformerMode)
                (inverted ? !realPlayer->m_isGoingLeft : realPlayer->m_isGoingLeft) ? fakePlayer->pushButton(static_cast<PlayerButton>(2)) : fakePlayer->pushButton(static_cast<PlayerButton>(3));
        }

        fakePlayer->update(t.delta);
        fakePlayer->updateRotation(t.delta);
        fakePlayer->updatePlayerScale();

        cocos2d::ccColor4F color = hold ? t.color1 : t.color2;

        if (!hold) {
            if ((player2 && t.player2Trajectory[i] == prevPos) || !player2 && t.player1Trajectory[i] == prevPos)
                color = t.color3;
        }

        if (i >= t.length - 40)
            color.a = (t.length - i) / 40.f;

        t.trajectoryNode()->drawSegment(prevPos, fakePlayer->getPosition(), 0.6f, color);
    }

}

void ShowTrajectory::drawPlayerHitbox(PlayerObject* player, CCDrawNode* drawNode) {
    cocos2d::CCRect bigRect = player->GameObject::getObjectRect();
    cocos2d::CCRect smallRect = player->GameObject::getObjectRect(0.3, 0.3);

    std::vector<cocos2d::CCPoint> vertices = ShowTrajectory::getVertices(player, bigRect, t.deathRotation);
    drawNode->drawPolygon(&vertices[0], 4, ccc4f(t.color2.r, t.color2.g, t.color2.b, 0.2f), 0.5, t.color2);

    vertices = ShowTrajectory::getVertices(player, smallRect, t.deathRotation);
    drawNode->drawPolygon(&vertices[0], 4, ccc4f(t.color3.r, t.color3.g, t.color3.b, 0.2f), 0.35, ccc4f(t.color3.r, t.color3.g, t.color3.b, 0.55f));
}

std::vector<cocos2d::CCPoint> ShowTrajectory::getVertices(PlayerObject* player, cocos2d::CCRect rect, float rotation) {
    std::vector<cocos2d::CCPoint> vertices = {
        ccp(rect.getMinX(), rect.getMaxY()),
        ccp(rect.getMaxX(), rect.getMaxY()),
        ccp(rect.getMaxX(), rect.getMinY()),
        ccp(rect.getMinX(), rect.getMinY())
    };

    cocos2d::CCPoint center = ccp(
        (rect.getMinX() + rect.getMaxX()) / 2.f,
        (rect.getMinY() + rect.getMaxY()) / 2.f
    );

    float size = static_cast<int>(rect.getMaxX() - rect.getMinX());

    if ((size == 18 || size == 5) && player->getScale() == 1) {
        for (auto& vertex : vertices) {
            vertex.x = center.x + (vertex.x - center.x) / 0.6f;
            vertex.y = center.y + (vertex.y - center.y) / 0.6f;
        }
    }

    if ((size == 7 || size == 30 || size == 29 || size == 9) && player->getScale() != 1) {
        for (auto& vertex : vertices) {
            vertex.x = center.x + (vertex.x - center.x) * 0.6;
            vertex.y = center.y + (vertex.y - center.y) * 0.6f;
        }
    }

    if (player->m_isDart) {
        for (auto& vertex : vertices) {
            vertex.x = center.x + (vertex.x - center.x) * 0.3f;
            vertex.y = center.y + (vertex.y - center.y) * 0.3f;
        }
    }

    float angle = CC_DEGREES_TO_RADIANS(rotation * -1.f);
    for (auto& vertex : vertices) {
        float x = vertex.x - center.x;
        float y = vertex.y - center.y;

        float xNew = center.x + (x * cos(angle)) - (y * sin(angle));
        float yNew = center.y + (x * sin(angle)) + (y * cos(angle));

        vertex.x = xNew;
        vertex.y = yNew;
    }

    return vertices;
}

void ShowTrajectory::updateMergedColor() {
    cocos2d::ccColor4F newColor = { 0.f, 0.f, 0.f, 1.f };

    newColor.r = (color1.r + color2.r) / 2;
    newColor.b = (color1.b + color2.b) / 2;
    newColor.g = (color1.g + color2.g) / 2;

    newColor.r = std::min(1.f, newColor.r + 0.45f);
    newColor.g = std::min(1.f, newColor.g + 0.45f);
    newColor.b = std::min(1.f, newColor.b + 0.45f);

    color3 = newColor;
}

void ShowTrajectory::handlePortal(PlayerObject* player, int id) {
    if (!portalIDs.contains(id)) return;

    switch (id) {
    case 101:
        player->togglePlayerScale(true, true);
        player->updatePlayerScale();
        break;
    case 99:
        player->togglePlayerScale(false, true);
        player->updatePlayerScale();
        break;
        // case 11:
            // player->flipGravity(true, true); break;
        // case 10:
            // player->flipGravity(false, true); break;
    case 200: player->m_playerSpeed = 0.7f; break;
    case 201: player->m_playerSpeed = 0.9f; break;
    case 202: player->m_playerSpeed = 1.1f; break;
    case 203: player->m_playerSpeed = 1.3f; break;
    case 1334: player->m_playerSpeed = 1.6f; break;
    }
}

cocos2d::CCDrawNode* ShowTrajectory::trajectoryNode() {

    static TrajectoryNode* instance = nullptr;

    if (!instance) {
        instance = TrajectoryNode::create();
        instance->retain();

        cocos2d::_ccBlendFunc  blendFunc;
        blendFunc.src = GL_SRC_ALPHA;
        blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;

        instance->setBlendFunc(blendFunc);
    }

    return instance;
}

class $modify(PlayLayer) {

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!t.trajectoryNode() || t.creatingTrajectory) return;

        if (Global::get().showTrajectory) {
            ShowTrajectory::updateTrajectory(this);
        }

    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        t.fakePlayer1 = nullptr;
        t.fakePlayer2 = nullptr;
        t.cancelTrajectory = false;
        t.creatingTrajectory = false;

        t.fakePlayer1 = PlayerObject::create(1, 1, this, this, true);
        t.fakePlayer1->retain();
        t.fakePlayer1->setPosition({ 0, 105 });
        t.fakePlayer1->setVisible(false);
        m_objectLayer->addChild(t.fakePlayer1);

        t.fakePlayer2 = PlayerObject::create(1, 1, this, this, true);
        t.fakePlayer2->retain();
        t.fakePlayer2->setPosition({ 0, 105 });
        t.fakePlayer2->setVisible(false);
        m_objectLayer->addChild(t.fakePlayer2);

        m_objectLayer->addChild(t.trajectoryNode(), 500);
    }

    void destroyPlayer(PlayerObject * player, GameObject * gameObject) {
        if (t.creatingTrajectory || (player == t.fakePlayer1 || player == t.fakePlayer2)) {
            t.deathRotation = player->getRotation();
            t.cancelTrajectory = true;
            return;
        }

        PlayLayer::destroyPlayer(player, gameObject);
    }

    void onQuit() {
        if (t.trajectoryNode())
            t.trajectoryNode()->clear();

        t.fakePlayer1 = nullptr;
        t.fakePlayer2 = nullptr;
        t.cancelTrajectory = false;
        t.creatingTrajectory = false;

        PlayLayer::onQuit();
    }

    void playEndAnimationToPos(cocos2d::CCPoint p0) {
        if (!t.creatingTrajectory)
            PlayLayer::playEndAnimationToPos(p0);
    }

};

class $modify(PauseLayer) {
    void goEdit() {
        if (t.trajectoryNode())
            t.trajectoryNode()->clear();

        t.fakePlayer1 = nullptr;
        t.fakePlayer2 = nullptr;
        t.cancelTrajectory = false;
        t.creatingTrajectory = false;

        PauseLayer::goEdit();
    }
};

class $modify(GJBaseGameLayer) {

    void collisionCheckObjects(PlayerObject * p0, gd::vector<GameObject*>*objects, int p2, float p3) {
        if (t.creatingTrajectory) {
            std::vector<GameObject*> disabledObjects;

            for (const auto& obj : *objects) {
                if (!obj) continue;

                if ((!objectTypes.contains(static_cast<int>(obj->m_objectType)) && !portalIDs.contains(obj->m_objectID)) || collectibleIDs.contains(obj->m_objectID)) {
                    if (obj->m_isDisabled || obj->m_isDisabled2) continue;  

                    disabledObjects.push_back(obj);
                    obj->m_isDisabled = true;
                    obj->m_isDisabled2 = true;
                }
            }

            GJBaseGameLayer::collisionCheckObjects(p0, objects, p2, p3);

            for (const auto& obj : disabledObjects) {
                if (!obj) continue;

                obj->m_isDisabled = false;
                obj->m_isDisabled2 = false;
            }

            disabledObjects.clear();

            return;
        }

        GJBaseGameLayer::collisionCheckObjects(p0, objects, p2, p3);
    }

    bool canBeActivatedByPlayer(PlayerObject * p0, EffectGameObject * p1) {
        if (t.creatingTrajectory) {

            ShowTrajectory::handlePortal(p0, p1->m_objectID);

            return false;
        }

        return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
    }

    void playerTouchedRing(PlayerObject * p0, RingObject * p1) {
        if (!t.creatingTrajectory)
            GJBaseGameLayer::playerTouchedRing(p0, p1);
    }

    void playerTouchedTrigger(PlayerObject * p0, EffectGameObject * p1) {
        if (!t.creatingTrajectory)
            GJBaseGameLayer::playerTouchedTrigger(p0, p1);
        else
            ShowTrajectory::handlePortal(p0, p1->m_objectID);
    }

    void activateSFXTrigger(SFXTriggerGameObject * p0) {
        if (!t.creatingTrajectory)
            GJBaseGameLayer::activateSFXTrigger(p0);

    }
    void activateSongEditTrigger(SongTriggerGameObject * p0) {
        if (!t.creatingTrajectory)
            GJBaseGameLayer::activateSongEditTrigger(p0);

    }
    // void activateSongTrigger(SongTriggerGameObject * p0) {
    //     if (!t.creatingTrajectory)
    //         GJBaseGameLayer::activateSongTrigger(p0);
    // }

    void gameEventTriggered(GJGameEvent p0, int p1, int p2) {
        if (!t.creatingTrajectory)
            GJBaseGameLayer::gameEventTriggered(p0, p1, p2);
    }

};

class $modify(PlayerObject) {

    void update(float dt) {
        PlayerObject::update(dt);
        t.delta = dt;
    }

    void playSpiderDashEffect(cocos2d::CCPoint p0, cocos2d::CCPoint p1) {
        if (!t.creatingTrajectory)
            PlayerObject::playSpiderDashEffect(p0, p1);
    }

    void incrementJumps() {
        if (!t.creatingTrajectory)
            PlayerObject::incrementJumps();
    }

    void ringJump(RingObject * p0, bool p1) {
        if (!t.creatingTrajectory)
            PlayerObject::ringJump(p0, p1);
    }

};

class $modify(HardStreak) {

    void addPoint(cocos2d::CCPoint p0) {
        if (!t.creatingTrajectory)
            HardStreak::addPoint(p0);
    }
};

class $modify(GameObject) {

    void playShineEffect() {
        if (!t.creatingTrajectory)
            GameObject::playShineEffect();
    }
};

class $modify(EffectGameObject) {

    void triggerObject(GJBaseGameLayer * p0, int p1, const gd::vector<int>*p2) {
        if (!t.creatingTrajectory)
            EffectGameObject::triggerObject(p0, p1, p2);
    }
};