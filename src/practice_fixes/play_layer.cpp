#include "practice_fixes.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CheckpointObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

class $modify(GJBaseGameLayer) {

  void toggleFlipped(bool p0, bool p1) {
    if (Mod::get()->getSavedValue<bool>("no_mirror_portal"))
      p0 = false;
    if (Global::get().state == state::recording || Mod::get()->getSavedValue<bool>("instant_mirror_portal"))
      p1 = true;

    GJBaseGameLayer::toggleFlipped(p0, p1);
  }

  void checkpointActivated(CheckpointGameObject * cp) {

    if (Global::get().state == state::none) 
      GJBaseGameLayer::checkpointActivated(cp);
    else
      Global::get().cancelCheckpoint = true;
  }

};

class $modify(CheckpointObject) {
  #ifdef GEODE_IS_WINDOWS
  bool init() {
    bool ret = CheckpointObject::init();
    CheckpointObject* cp = this;
  #else
  static CheckpointObject* create() {
    CheckpointObject* ret = CheckpointObject::create();
    CheckpointObject* cp = ret;
  #endif

    if (!cp) return ret;

    auto& g = Global::get();
    PlayLayer* pl = PlayLayer::get();

    PlayerData p1Data = PlayerPracticeFixes::saveData(pl->m_player1);
    PlayerData p2Data = PlayerPracticeFixes::saveData(pl->m_player2);

    Global::get().checkpoints[cp] = {
      Global::getCurrentFrame(),
      p1Data,
      p2Data,
      #ifdef GEODE_IS_WINDOWS
      *(uintptr_t*)((char*)geode::base::get() + seedAddr),
      #else
      0,
      #endif
      Global::get().previousFrame
    };

    return ret;

  }
};

class $modify(PlayLayer) {

  void storeCheckpoint(CheckpointObject * cp) {
    if (!cp) return PlayLayer::storeCheckpoint(cp);

    auto& g = Global::get();

    if (!g.cancelCheckpoint)
      PlayLayer::storeCheckpoint(cp);
    else {
      g.cancelCheckpoint = false;
      return;
    }

  }

  void loadFromCheckpoint(CheckpointObject* cp) {
    
    if (!cp) return PlayLayer::loadFromCheckpoint(cp);

    auto& g = Global::get();

    Macro::tryAutosave(m_level, cp);

    if (g.state == state::playing) {
      PlayLayer::loadFromCheckpoint(cp);

      if (!g.checkpoints.contains(cp)) return;
      if (g.checkpoints[cp].frame <= 1) return;

      PlayerData p1Data = g.checkpoints[cp].p1;
      PlayerData p2Data = g.checkpoints[cp].p2;

      g.respawnFrame = g.checkpoints[cp].frame;
      g.previousFrame = g.checkpoints[cp].previousFrame;
      PlayerPracticeFixes::applyData(this->m_player1, p1Data, false);
      PlayerPracticeFixes::applyData(this->m_player2, p2Data, true);

      return;
    }

    if ((g.state != state::recording && !Mod::get()->getSavedValue<bool>("macro_always_practice_fixes")))
      return PlayLayer::loadFromCheckpoint(cp);

    if (!g.checkpoints.contains(cp)) return PlayLayer::loadFromCheckpoint(cp);

    Macro::resetVariables();

    int frame = g.checkpoints[cp].frame;
    PlayerData p1Data = g.checkpoints[cp].p1;
    PlayerData p2Data = g.checkpoints[cp].p2;

    g.ignoreJumpButton = frame + 1;
    g.previousFrame = g.checkpoints[cp].previousFrame;

    #ifdef GEODE_IS_WINDOWS

    if (g.seedEnabled) {
      uintptr_t seed = g.checkpoints[cp].seed;
      *(uintptr_t*)((char*)geode::base::get() + seedAddr) = seed;
    }

    #endif

    if (g.state == state::recording)
      InputPracticeFixes::applyFixes(this, p1Data, p2Data, frame);

    PlayLayer::loadFromCheckpoint(cp);

    PlayerPracticeFixes::applyData(this->m_player1, p1Data, false);
    PlayerPracticeFixes::applyData(this->m_player2, p2Data, true);

    if (g.state != state::recording && g.mod->getSavedValue<bool>("macro_always_practice_fixes")) {
      this->m_player1->releaseButton(static_cast<PlayerButton>(1));
      this->m_player2->releaseButton(static_cast<PlayerButton>(1));
    }

  }

};
