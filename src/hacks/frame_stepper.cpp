#include "../includes.hpp"

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCParticleSystem.hpp>  
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#ifdef GEODE_IS_WINDOWS

class $modify(PauseLayer) {

    void onResume(CCObject * sender) {
        PauseLayer::onResume(sender);

        if (Global::get().frameStepper) {
            Global::get().stepFrameDrawMultiple = 4;
        }
    }

};

class $modify(CCDirector) {

    static void onModify(auto & self) {
        if (!self.setHookPriority("CCDirector::drawScene", -1))
            log::warn("CCDirector::drawScene hook priority fail xD.");
    }

    void drawScene() {
        auto& g = Global::get();

        if (!g.frameStepper) return CCDirector::drawScene();

        PlayLayer* pl = PlayLayer::get();

        if (!pl) return CCDirector::drawScene();

        if (pl->m_isPaused || pl->m_player1->m_isDead || Global::getCurrentFrame() < 3) return CCDirector::drawScene();

        if (g.stepFrameDraw || g.stepFrameDrawMultiple != 0) {
            g.stepFrameDraw = false;

            if (g.stepFrameDrawMultiple != 0)
                g.stepFrameDrawMultiple--;

            CCDirector::drawScene();
        }
        else
            this->getScheduler()->update(1.f / 240.f);

    }
};

#endif

class $modify(PlayLayer) {

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        if (Global::get().frameStepper)
            Global::toggleFrameStepper();
    }

};

class $modify(GJBaseGameLayer) {

    static void onModify(auto & self) {
        if (!self.setHookPriority("GJBaseGameLayer::update", 100))
            log::warn("GJBaseGameLayer::update hook priority fail xD.");
    }

    void update(float dt) {
        if (this->m_player1->m_isDead) {
            if (Global::get().mod->getSavedValue<bool>("macro_instant_respawn"))
                PlayLayer::get()->resetLevel();
        }

        if (!PlayLayer::get()) return GJBaseGameLayer::update(dt);

        auto& g = Global::get();

        if (!g.renderer.recording && g.frameStepper) {
            if (g.stepFrameParticle != 0)
                g.stepFrameParticle--;

            if (Macro::shouldStep()) {
                g.stepFrame = false;

                GJBaseGameLayer::update(1.f / 240.f);

                g.frameStepperMusicTime = FMODAudioEngine::sharedEngine()->getMusicTimeMS(0);

                return;
            }
            else {
                g.safeMode = true;
                return;
            }
        }

        GJBaseGameLayer::update(dt);
    }

};

class $modify(CCParticleSystem) {

    static void onModify(auto & self) {
        if (!self.setHookPriority("CCParticleSystem::update", -1))
            log::warn("CCParticleSystem::update hook priority fail xD.");
    }

    virtual void update(float dt) {
        auto& g = Global::get();
        if (!PlayLayer::get()) return CCParticleSystem::update(dt);

        int frame = Global::getCurrentFrame();

        if (!g.renderer.recording && g.frameStepper) {
            if (g.stepFrameParticle != 0) {
                CCParticleSystem::update(dt);
            }
            else
                return;
        }

        CCParticleSystem::update(dt);
    }
};
