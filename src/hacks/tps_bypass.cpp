#include "../includes.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>

class $modify(GJBaseGameLayer) {

    struct Fields {
        float leftOver = 0.f;
    };

    void update(float dt) {
        if (!Global::get().tpsEnabled) return GJBaseGameLayer::update(dt);
        if (Global::getTPS() == 240.f) return GJBaseGameLayer::update(dt);
        if (!PlayLayer::get()) return GJBaseGameLayer::update(dt);
        if (Global::get().frameStepper) return GJBaseGameLayer::update(dt);

        using namespace std::literals;
        
        float newDt = 1.f / Global::getTPS();

        auto startTime = std::chrono::high_resolution_clock::now();
        int mult = static_cast<int>((dt + m_fields->leftOver) / newDt);

        for (int i = 0; i < mult; ++i) {
            GJBaseGameLayer::update(newDt);
            if (std::chrono::high_resolution_clock::now() - startTime > 33.333ms) {
                mult = i + 1;
                break;
            }
        }
        
        m_fields->leftOver += (dt - newDt * mult);
    }

    float getModifiedDelta(float dt) {
        if (!Global::get().tpsEnabled) return GJBaseGameLayer::getModifiedDelta(dt);
        if (Global::getTPS() == 240.f) return GJBaseGameLayer::getModifiedDelta(dt);
        if (!PlayLayer::get()) return GJBaseGameLayer::getModifiedDelta(dt);

        double dVar1;
        float fVar2;
        float fVar3;
        double dVar4;

        float newDt = 1.f / Global::getTPS();
        
        if (0 < m_resumeTimer) {
            // cocos2d::CCDirector::sharedDirector();
            m_resumeTimer = m_resumeTimer + -1;
            dt = 0.0;
        }

        fVar2 = 1.0;
        if (m_gameState.m_timeWarp <= 1.0) {
            fVar2 = m_gameState.m_timeWarp;
        }

        dVar1 = dt + m_extraDelta;
        fVar3 = std::roundf(dVar1 / (fVar2 * newDt));
        dVar4 = fVar3 * fVar2 * newDt;
        m_extraDelta = dVar1 - dVar4;

        return dVar4;
    }

};