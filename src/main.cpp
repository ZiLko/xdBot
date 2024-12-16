#include "includes.hpp"

#include "ui/record_layer.hpp"
#include "practice_fixes/practice_fixes.hpp"
#include "hacks/layout_mode.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

$execute {

  geode::listenForSettingChanges("frame_fixes", +[](bool value) {
    Global::get().frameFixes = value;
  });

};

class $modify(PlayLayer) {

  struct Fields {
    int delayedLevelRestart = -1;
  };

  // void postUpdate(float dt) { 
  //   PlayLayer::postUpdate(dt);
  //   auto& g = Global::get();

  //   if (m_fields->delayedLevelRestart != -1 && m_fields->delayedLevelRestart >= Global::getCurrentFrame()) {
  //     m_fields->delayedLevelRestart = -1;
  //     this->resetLevelFromStart();
  //   }

  // }

  void pauseGame(bool b1) {
    Global::updateKeybinds();

    auto& g = Global::get();

    if (!m_player1 || !m_player2) return PlayLayer::pauseGame(b1);

    if (g.state == state::playing) {
      if (m_player1->m_isShip || m_player1->m_isDart) {
        g.wasHolding[0] = m_player1->m_holdingButtons[1];
        g.wasHolding[1] = m_player1->m_holdingButtons[2];
        g.wasHolding[2] = m_player1->m_holdingButtons[3];
      }

      if (m_player2->m_isShip || m_player2->m_isDart) {
        g.wasHolding[3] = m_player2->m_holdingButtons[1];
        g.wasHolding[4] = m_player2->m_holdingButtons[2];
        g.wasHolding[5] = m_player2->m_holdingButtons[3];
      }
    }

    if (g.state != state::recording) return PlayLayer::pauseGame(b1);

    g.ignoreRecordAction = true;
    int frame = Global::getCurrentFrame() + 1;

    if (m_player1->m_holdingButtons[1]) {
      this->handleButton(false, 1, false);
      g.macro.inputs.push_back(input(frame, 1, false, false));
    }
    if (this->m_levelSettings->m_platformerMode) {
      if (m_player1->m_holdingButtons[2]) {
        this->handleButton(false, 2, false);
        g.macro.inputs.push_back(input(frame, 2, false, false));
      }
      if (m_player1->m_holdingButtons[3]) {
        this->handleButton(false, 3, false);
        g.macro.inputs.push_back(input(frame, 3, false, false));
      }
    }

    if (this->m_levelSettings->m_twoPlayerMode) {
      if (m_player2->m_holdingButtons[1]) {
        this->handleButton(false, 1, true);
        g.macro.inputs.push_back(input(frame, 1, true, false));
      }
      if (this->m_levelSettings->m_platformerMode) {
        if (m_player2->m_holdingButtons[2]) {
          this->handleButton(false, 2, false);
          g.macro.inputs.push_back(input(frame, 2, true, false));
        }
        if (m_player2->m_holdingButtons[3]) {
          this->handleButton(false, 3, false);
          g.macro.inputs.push_back(input(frame, 3, true, false));
        }
      }
    }

    g.ignoreRecordAction = false;

    PlayLayer::pauseGame(b1);

  }

  bool init(GJGameLevel * level, bool b1, bool b2) {
    if (!PlayLayer::init(level, b1, b2)) return false;

    auto& g = Global::get();

    g.firstAttempt = true;

    Global::updateKeybinds();

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    g.currentSession = std::to_string(timestamp);
    g.lastAutoSave = 0;

    return true;
  }

  void resetLevel() {
    PlayLayer::resetLevel();

    auto& g = Global::get();

    int frame = Global::getCurrentFrame();

    if (!m_isPracticeMode)
      g.renderer.levelStartFrame = frame;

    // if (g.restart && m_levelSettings->m_platformerMode && g.state != state::none)
    //   m_fields->delayedLevelRestart = frame + 1;

    Global::updateSeed(true);

    g.safeMode = false;

    if (g.layoutMode)
      g.safeMode = true;

    g.currentAction = 0;
    g.currentFrameFix = 0;
    g.restart = false;

    if (g.state == state::recording)
      Macro::updateInfo(this);

    if ((!this->m_isPracticeMode || frame == 0 || g.checkpoints.empty()) && g.state == state::recording) {
      Macro::resetVariables();

      g.macro.inputs.clear();
      g.macro.frameFixes.clear();
      g.checkpoints.clear();

      g.macro.canChangeFPS = true;

      PlayerData p1Data = PlayerPracticeFixes::saveData(this->m_player1);
      PlayerData p2Data = PlayerPracticeFixes::saveData(this->m_player2);

      InputPracticeFixes::applyFixes(this, p1Data, p2Data, frame);

      m_player1->m_holdingRight = false;
      m_player1->m_holdingLeft = false;
      m_player2->m_holdingRight = false;
      m_player2->m_holdingLeft = false;

      m_player1->m_holdingButtons[2] = false;
      m_player1->m_holdingButtons[3] = false;
      m_player2->m_holdingButtons[2] = false;
      m_player2->m_holdingButtons[3] = false;
    }

    if (!this->m_levelSettings->m_platformerMode || (!g.mod->getSavedValue<bool>("macro_always_practice_fixes") && g.state != state::recording)) return;

    g.ignoreRecordAction = true;
    for (int i = 0; i < 4; i++) {
      bool player2 = !(sidesButtons[i] > 2);
      bool rightKey = sidesButtons[i] == 5 || sidesButtons[i] == 2;
      if (g.heldButtons[sidesButtons[i]])
        this->handleButton(true, indexButton[sidesButtons[i]], player2);
    }
    g.ignoreRecordAction = false;
  }

  void levelComplete() {
    PlayLayer::levelComplete();

    Global::get().firstAttempt = true;

    Macro::resetState(true);
  }

};

class $modify(BGLHook, GJBaseGameLayer) {

  struct Fields {
    bool macroInput = false;
  };

  void processCommands(float dt) {
    auto& g = Global::get();

    PlayLayer* pl = PlayLayer::get();

    if (!pl)
      return GJBaseGameLayer::processCommands(dt);

    Global::updateSeed();

    int frame = Global::getCurrentFrame();

    bool rendering = g.renderer.recording || g.renderer.recordingAudio;

    if (g.state != state::none || rendering) {

      if (!g.firstAttempt) {
        g.renderer.dontRender = false;
        g.renderer.dontRecordAudio = false;
      }

      if (frame > 10 && g.firstAttempt) {
        g.firstAttempt = false;

        if ((m_levelSettings->m_platformerMode || rendering) && !m_levelEndAnimationStarted)
          return PlayLayer::get()->resetLevelFromStart();
        else if (!m_levelEndAnimationStarted)
          return PlayLayer::get()->resetLevel();
      }

      // if (g.previousFrame == frame && frame != 0)
        // return;

    }

    GJBaseGameLayer::processCommands(dt);

    g.previousFrame = frame;

    if (g.state == state::none)
      return;

    if (g.restart && !m_levelEndAnimationStarted) {
      if ((m_levelSettings->m_platformerMode && g.state != state::none) || g.renderer.recording || g.renderer.recordingAudio)
        return pl->resetLevelFromStart();
      else
        return pl->resetLevel();
    }

    if (g.state == state::recording)
      handleRecording(frame);

    if (g.state == state::playing)
      handlePlaying(frame);

  }

  void handleRecording(int frame) {
    auto& g = Global::get();

    if (g.ignoreFrame != -1) {
      if (g.ignoreFrame < frame) g.ignoreFrame = -1;
    }

    bool twoPlayers = m_levelSettings->m_twoPlayerMode;

    if (g.delayedFrameInput[0] == frame) {
      g.delayedFrameInput[0] = -1;
      if ((g.heldButtons[0] && twoPlayers) || (!twoPlayers && (g.heldButtons[0] || g.heldButtons[3])))
        GJBaseGameLayer::handleButton(true, 1, true);
    }

    if (g.delayedFrameInput[1] == frame) {
      g.delayedFrameInput[1] = -1;
      if ((g.heldButtons[3] && twoPlayers) || (!twoPlayers && (g.heldButtons[0] || g.heldButtons[3])))
        GJBaseGameLayer::handleButton(true, 1, false);
    }

    if (frame > g.ignoreJumpButton && g.ignoreJumpButton != -1)
      g.ignoreJumpButton = -1;

    for (int x = 0; x < 2; x++) {
      if (g.delayedFrameReleaseMain[x] == frame) {
        bool player2 = x == 0;
        g.delayedFrameReleaseMain[x] = -1;
        GJBaseGameLayer::handleButton(false, 1, twoPlayers ? player2 : false);
      }

      if (!this->m_levelSettings->m_platformerMode)
        continue;

      for (int y = 0; y < 2; y++) {
        if (g.delayedFrameRelease[x][y] == frame) {
          int button = y == 0 ? 2 : 3;
          bool player2 = x == 0;
          g.delayedFrameRelease[x][y] = -1;
          GJBaseGameLayer::handleButton(false, button, player2);
        }
      }
    }

    if (!g.frameFixes) return;

    float p1Rotation = m_player1->getRotation();
    float p2Rotation = m_player2->getRotation();

    // Just in case

    while (p1Rotation < 0 || p1Rotation > 360)
      p1Rotation += p1Rotation < 0 ? 360.f : -360.f;
    
    while (p2Rotation < 0 || p2Rotation > 360)
      p2Rotation += p2Rotation < 0 ? 360.f : -360.f;

    g.macro.frameFixes.push_back({
      frame,
      { m_player1->getPosition(), p1Rotation },
      { m_player2->getPosition(), p2Rotation }
    });

  }

  void handlePlaying(int frame) {
    auto& g = Global::get();
    if (m_levelEndAnimationStarted) return;

    if (m_player1->m_isDead) {
      m_player1->releaseAllButtons();
      m_player2->releaseAllButtons();
      return;
    }

    m_fields->macroInput = true;

    while (g.currentAction < g.macro.inputs.size() && frame >= g.macro.inputs[g.currentAction].frame) {
      auto& input = g.macro.inputs[g.currentAction];

      if (frame != g.respawnFrame) {
        GJBaseGameLayer::handleButton(input.down, input.button, input.player2);
      }

      g.currentAction++;
      g.safeMode = true;
    }

    g.respawnFrame = -1;
    m_fields->macroInput = false;

    if (g.currentAction == g.macro.inputs.size()) {
      if (g.mod->getSavedValue<bool>("macro_auto_stop_playing")) {
        Macro::togglePlaying();
        Macro::resetState(true);

        return;
      }
    }

    if (!g.frameFixes) return;

    while (g.currentFrameFix < g.macro.frameFixes.size() && frame >= g.macro.frameFixes[g.currentFrameFix].frame) {
      auto& fix = g.macro.frameFixes[g.currentFrameFix];

      PlayerObject* p1 = m_player1;
      PlayerObject* p2 = m_player2;

      cocos2d::CCPoint pos1 = p1->getPosition();
      cocos2d::CCPoint pos2 = p2->getPosition();

      if (fix.p1.pos.x != 0.f && fix.p1.pos.y != 0.f)
        p1->setPosition(fix.p1.pos);
        
      if (fix.p1.rotate && fix.p1.rotation != 0.f)
        p1->setRotation(fix.p1.rotation);

      if (m_gameState.m_isDualMode) {
        if (fix.p2.pos.x != 0.f && fix.p2.pos.y != 0.f)
          p2->setPosition(fix.p2.pos);

        if (fix.p2.rotate && fix.p2.rotation != 0.f)
          p2->setRotation(fix.p2.rotation);
      }

      g.currentFrameFix++;
    }

  }

  void handleButton(bool hold, int button, bool player2) {
    auto& g = Global::get();

    if (g.p2mirror && m_gameState.m_isDualMode)
      GJBaseGameLayer::handleButton(hold, button, !player2);

    if (g.state == state::none)
      return GJBaseGameLayer::handleButton(hold, button, player2);

    if (g.state == state::playing) {
      if (g.mod->getSavedValue<bool>("macro_ignore_inputs") && !m_fields->macroInput)
        return;
      else return GJBaseGameLayer::handleButton(hold, button, player2);

    }
    else if (g.ignoreFrame != -1 && hold)
      return;

    int frame = Global::getCurrentFrame();

    if (frame >= 10 && hold)
      Global::hasIncompatibleMods();

    bool isDelayedInput = g.delayedFrameInput[(this->m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] != -1;
    bool isDelayedRelease = g.delayedFrameReleaseMain[(this->m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] != -1;

    if ((isDelayedInput || g.ignoreJumpButton == frame || isDelayedRelease) && button == 1) {
      if (g.ignoreJumpButton >= frame)
        g.delayedFrameInput[(this->m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] = g.ignoreJumpButton + 1;

      return;
    }

    GJBaseGameLayer::handleButton(hold, button, player2);

    if (g.state != state::recording) return;

    if (!this->m_levelSettings->m_twoPlayerMode)
      player2 = false;

    if (!g.ignoreRecordAction && !g.creatingTrajectory && !m_player1->m_isDead) {
      g.macro.recordAction(frame, button, player2, hold);
      if (g.p2mirror && m_gameState.m_isDualMode)
        g.macro.recordAction(frame, button, !player2, hold);
    }

  }
};

class $modify(PauseLayer) {

  void onPracticeMode(CCObject * sender) {
    PauseLayer::onPracticeMode(sender);
    if (Global::get().state != state::none) PlayLayer::get()->resetLevel();
  }

  void onNormalMode(CCObject * sender) {
    PauseLayer::onNormalMode(sender);
    auto& g = Global::get();

    g.checkpoints.clear();

    if (g.restart) {
      g.restart = false;
      if (PlayLayer* pl = PlayLayer::get())
        pl->resetLevel();
    }

  }

  void onQuit(CCObject * sender) {
    PauseLayer::onQuit(sender);
    auto& g = Global::get();
    Macro::resetState();
  }

  void goEdit() {
    PauseLayer::goEdit();
    auto& g = Global::get();
    Macro::resetState();
  }

  void onResume(CCObject * sender) {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    PauseLayer::onResume(sender);

    auto& g = Global::get();

    if (g.state == state::playing) {
      for (int i = 0; i < 6; i++) {
        if (!g.wasHolding[i]) continue;
        BGLHook* bgl = typeinfo_cast<BGLHook*>(pl->m_player1->m_gameLayer);

        if (bgl)
          bgl->m_fields->macroInput = true;

        pl->handleButton(true, indexButton[i], i >= 3);
        g.wasHolding[i] = false;

        if (bgl)
          bgl->m_fields->macroInput = false;
      }
    }

    if (!g.restart) return;

    if ((pl->m_levelSettings->m_platformerMode && g.state != state::none) || g.renderer.recording || g.renderer.recordingAudio)
      return pl->resetLevelFromStart();
    else
      return pl->resetLevel();

    g.restart = false;

  }
};
