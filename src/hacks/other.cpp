#include "../includes.hpp"
#include "../ui/record_layer.hpp"
#include "../ui/load_macro_layer.hpp"
#include "../ui/save_macro_layer.hpp"
#include "../ui/clickbot_layer.hpp"
#include "../ui/macro_info_layer.hpp"
#include "../ui/render_settings_layer.hpp"

#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/Geode.hpp>

class $modify(CCScheduler) {

    void update(float dt) {
        auto& g = Global::get();

        if (g.state == state::none && !g.speedhackEnabled) {
            if (g.currentPitch != 1.f)
                Global::updatePitch(1.f);

            return CCScheduler::update(dt);
        }

        if (!PlayLayer::get() || g.renderer.recording || g.renderer.recordingAudio) {
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

        if (speedhack != 1.f)
            g.safeMode = true;

        CCScheduler::update(dt * speedhack);
    }

};

class $modify(PlayerObject) {

    static void onModify(auto & self) {
        if (!self.setHookPriority("PlayerObject::playDeathEffect", -1))
            log::warn("PlayerObject::playDeathEffect hook priority fail xD.");

        if (!self.setHookPriority("PlayerObject::playSpawnEffect", -1))
            log::warn("PlayerObject::playSpawnEffect hook priority fail xD.");
    }

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

    static void onModify(auto & self) {
        if (!self.setHookPriority("PlayLayer::destroyPlayer", -1))
            log::warn("PlayLayer::destroyPlayer hook priority fail xD.");

        if (!self.setHookPriority("PlayLayer::showNewBest", -1))
            log::warn("PlayLayer::showNewBest hook priority fail xD.");
    }

    void destroyPlayer(PlayerObject * p0, GameObject * p1) {
        if (p0 != m_player1 && p0 != m_player2) return PlayLayer::destroyPlayer(p0, p1);
        
        if (!m_fields->slopeFix)
            m_fields->slopeFix = p1;

        auto& g = Global::get();

        bool player2 = p0 == m_player2;

        if (!g.mod->getSavedValue<bool>("macro_noclip_p1") && !player2)
            return PlayLayer::destroyPlayer(p0, p1);

        if (!g.mod->getSavedValue<bool>("macro_noclip_p2") && player2)
            return PlayLayer::destroyPlayer(p0, p1);

        if (!g.mod->getSavedValue<bool>("macro_noclip") || m_fields->slopeFix == p1)
            PlayLayer::destroyPlayer(p0, p1);
        else
            Global::get().safeMode = true;
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

        if (g.layer) {
            static_cast<RecordLayer*>(g.layer)->cursorWasHidden = false;
            static_cast<RecordLayer*>(g.layer)->onClose(nullptr);
        }

        if (!g.safeMode) return;
        if (!g.mod->getSavedValue<bool>("macro_auto_safe_mode")) return;

        CCLabelBMFont* lbl = CCLabelBMFont::create("Auto-safe-mode", "goldFont.fnt");
        lbl->setPosition({ 3.5, 5 });
        lbl->setOpacity(170);
        lbl->setID("safe-mode-label"_spr);
        lbl->setScale(0.55f);
        lbl->setAnchorPoint({ 0, 0.5 });

        this->getChildByID("main-layer")->addChild(lbl);

    }

};

class $modify(GJGameLevel) {

    static void onModify(auto & self) {
        if (!self.setHookPriority("GJGameLevel::savePercentage", -1))
            log::warn("GJGameLevel::savePercentage hook priority fail xD.");
    }

    void savePercentage(int p0, bool p1, int p2, int p3, bool p4) {
        if (!Global::get().safeMode || !Mod::get()->getSavedValue<bool>("macro_auto_safe_mode"))
            GJGameLevel::savePercentage(p0, p1, p2, p3, p4);
    }
};