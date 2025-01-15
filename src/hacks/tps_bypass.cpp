#include "../includes.hpp"
#include <Geode/modify/GJBaseGameLayer.hpp>

float leftOver2 = 0.f;

class $modify(GJBaseGameLayer) {

    void update(float dt) {
        if (!Global::get().tpsEnabled) return GJBaseGameLayer::update(dt);
        if (Global::getTPS() == 240.f) return GJBaseGameLayer::update(dt);
        if (!PlayLayer::get()) return GJBaseGameLayer::update(dt);
        
        float newDt = 1.f / Global::getTPS();

        if (Global::get().frameStepper) return GJBaseGameLayer::update(newDt);

        float realDt = dt + leftOver2;
        if (realDt > dt) realDt = dt;

        auto startTime = std::chrono::high_resolution_clock::now();
        int mult = static_cast<int>(realDt / newDt);

        for (int i = 0; i < mult; ++i) {
            GJBaseGameLayer::update(newDt);
            if (std::chrono::high_resolution_clock::now() - startTime > std::chrono::duration<double, std::milli>(16.666f)) {
                mult = i + 1;
                break;
            }
        }

        leftOver2 += (dt - newDt * mult);
        
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
            m_resumeTimer--;
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