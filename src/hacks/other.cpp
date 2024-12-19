#include "../includes.hpp"
#include "../ui/record_layer.hpp"

#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameLevelOptionsLayer.hpp>

class $modify(CCScheduler) {

    void update(float dt) {
        auto& g = Global::get();

        if (g.state == state::none && !g.speedhackEnabled) {
            if (g.currentPitch != 1.f)
                Global::updatePitch(1.f);

            return CCScheduler::update(dt);
        }

        if (g.renderer.recording || g.renderer.recordingAudio) {
            if (g.currentPitch != 1.f)
                Global::updatePitch(1.f);

            return CCScheduler::update(dt);
        }

        float speedhack = 1.f;

        if (g.speedhackEnabled && !g.frameStepper) {
            std::string speedhackValue = g.mod->getSavedValue<std::string>("macro_speedhack");

            if (speedhackValue != "0.0" && speedhackValue != "") {
                speedhack = std::stof(speedhackValue);
                float decimals = speedhack - static_cast<int>(speedhack);

                float closest = safeValues[0];
                float minDiff = std::abs(decimals - closest);

                for (float value : safeValues) {
                    if (speedhackValue == "1.0" || g.state == state::none) {
                        closest = decimals;
                        break;
                    }

                    float diff = std::abs(decimals - value);

                    if (diff < minDiff) {
                        minDiff = diff;
                        closest = value;
                    }
                }

                speedhack = static_cast<int>(speedhack) + closest;
            }

            Global::updatePitch(speedhack);
        }

        if (speedhack != 1.f && PlayLayer::get())
            g.safeMode = true;

        CCScheduler::update(dt * speedhack);
    }

};

class $modify(PlayerObject) {

    void playDeathEffect() {
        if (!Global::get().mod->getSavedValue<bool>("macro_no_death_effect"))
            PlayerObject::playDeathEffect();
    }

    void playSpawnEffect() {
        if (!Global::get().mod->getSavedValue<bool>("macro_no_respawn_flash"))
            PlayerObject::playSpawnEffect();
    }

};

class $modify(PlayLayer) {

    struct Fields {
        CCObject* slopeFix = nullptr;
    };

    void destroyPlayer(PlayerObject * p0, GameObject * p1) {
        if (p0 != m_player1 && p0 != m_player2) return PlayLayer::destroyPlayer(p0, p1);
        
        if (!m_fields->slopeFix)
            m_fields->slopeFix = p1;

        auto& g = Global::get();

        bool player2 = p0 == m_player2;

        if (!g.mod->getSavedValue<bool>("macro_noclip_p1") && !player2)
            PlayLayer::destroyPlayer(p0, p1);
        else if (!g.mod->getSavedValue<bool>("macro_noclip_p2") && player2)
            PlayLayer::destroyPlayer(p0, p1);
        else if (!g.mod->getSavedValue<bool>("macro_noclip") || m_fields->slopeFix == p1)
            PlayLayer::destroyPlayer(p0, p1);
        else
            Global::get().safeMode = true;
        
        if (getActionByTag(16)) {
            if (Global::get().renderer.recordingAudio) Global::get().renderer.stopAudio();
            
            if (g.mod->getSavedValue<bool>("respawn_time_enabled")) {
                stopActionByTag(16);
                CCSequence* seq = CCSequence::create(CCDelayTime::create(g.mod->getSavedValue<double>("respawn_time")), CCCallFunc::create(this, callfunc_selector(PlayLayer::delayedResetLevel)), nullptr);
                seq->setTag(16);
                runAction(seq);
            }
        }
    }

    void showNewBest(bool po, int p1, int p2, bool p3, bool p4, bool p5) {
        if (!Global::get().safeMode || !Mod::get()->getSavedValue<bool>("macro_auto_safe_mode"))
            PlayLayer::showNewBest(po, p1, p2, p3, p4, p5);
    };

    void levelComplete() {
        auto& g = Global::get();

        bool wasTestMode = m_isTestMode;

        if (g.safeMode && g.mod->getSavedValue<bool>("macro_auto_safe_mode"))
            m_isTestMode = true;

        if (m_isPracticeMode)
            g.safeMode = false;

        PlayLayer::levelComplete();
        Macro::resetState(true);

        m_isTestMode = wasTestMode;
    }

};

class $modify(EndLevelLayer) {
    
    void customSetup() {
        EndLevelLayer::customSetup();
        auto& g = Global::get();

        if (g.mod->getSettingValue<bool>("endscreen_button")) {
			cocos2d::CCSize winSize = CCDirector::sharedDirector()->getWinSize();

			CCSprite* sprite = CCSprite::createWithSpriteFrameName("GJ_playbtn_001.png");
			sprite->setScale(0.350f);
            
        	CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
                sprite,
			    this,
			    menu_selector(RecordLayer::openMenu2
            ));
			btn->setPosition({160, -99});

			CCLayer* layer = this->getChildByType<CCLayer>(0);

			CCMenu* menu = CCMenu::create();
            menu->setID("button-menu"_spr);
			layer->addChild(menu);

        	menu->addChild(btn);
		}

        if (g.layer) {
            static_cast<RecordLayer*>(g.layer)->cursorWasHidden = false;
            static_cast<RecordLayer*>(g.layer)->onClose(nullptr);
        }

        if (!g.safeMode) return;

        if (CCMenu* menu = m_mainLayer->getChildByType<CCMenu>(0)) {
            if (CCMenuItemSpriteExtra* btn = menu->getChildByType<CCMenuItemSpriteExtra>(0))
                btn->setPositionX(btn->getPositionX() - 2);
            if (CCMenuItemSpriteExtra* btn = menu->getChildByType<CCMenuItemSpriteExtra>(1))
                btn->setPositionX(btn->getPositionX() + 2);
            if (CCMenuItemSpriteExtra* btn = menu->getChildByType<CCMenuItemSpriteExtra>(2))
                btn->setPositionX(btn->getPositionX() - 2);
        }

        if (!g.mod->getSavedValue<bool>("macro_auto_safe_mode")) return;

        CCLabelBMFont* lbl = CCLabelBMFont::create("Auto-safe-mode", "goldFont.fnt");
        lbl->setPosition({ 3.5, 10 });
        lbl->setOpacity(155);
        lbl->setID("safe-mode-label"_spr);
        lbl->setScale(0.55f);
        lbl->setAnchorPoint({ 0, 0.5 });

        addChild(lbl);
    }

    void onHideLayer(CCObject* obj) {
        EndLevelLayer::onHideLayer(obj);

        if (CCNode* lbl = getChildByID("safe-mode-label"_spr))
            lbl->setVisible(!lbl->isVisible());
    }

};

class $modify(GJGameLevel) {

    void savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
        if (!Global::get().safeMode || !Mod::get()->getSavedValue<bool>("macro_auto_safe_mode"))
            GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
};

class $modify(EffectGameObject) {

    void triggerObject(GJBaseGameLayer* p0, int p1, gd::vector<int> const* p2) {
        if (!Mod::get()->getSavedValue<bool>("disable_shaders"))
            return EffectGameObject::triggerObject(p0, p1, p2);

        int id = m_objectID;
        if (id == 30 || id == 1006 || id == 105 || id == 915 || id == 29 || id == 58 || id == 56 || id == 1007 || id == 899)
            return;

        EffectGameObject::triggerObject(p0, p1, p2);
	}

};

class $modify(GameLevelOptionsLayer) {

    static GameLevelOptionsLayer* create(GJGameLevel* level) {
        GameLevelOptionsLayer* ret = GameLevelOptionsLayer::create(level);

        if (!Mod::get()->getSettingValue<bool>("level_settings_button")) return ret;

        CCSprite* sprite = CCSprite::createWithSpriteFrameName("GJ_playbtn_001.png");
		sprite->setScale(0.350f);
            
        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
            sprite,
		    ret,
		    menu_selector(RecordLayer::openMenu2)
        );
		btn->setPosition({-174, -114});

        if (CCLayer* layer = ret->getChildByType<CCLayer>(0))
            if (CCMenu* menu = layer->getChildByType<CCMenu>(1))
                menu->addChild(btn);

        return ret;
    }
    
};