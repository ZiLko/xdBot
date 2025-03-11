#include "includes.hpp"

#include "ui/record_layer.hpp"
#include "practice_fixes/practice_fixes.hpp"
#include "hacks/layout_mode.hpp"

#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

$execute {

  geode::listenForSettingChanges("macro_accuracy", +[](std::string value) {
    auto& g = Global::get();
    
    g.frameFixes = false;
    g.inputFixes = false;

    if (value == "Frame Fixes") g.frameFixes = true;
    if (value == "Input Fixes") g.inputFixes = true;
  });

  geode::listenForSettingChanges("frame_fixes_limit", +[](int64_t value) {
    Global::get().frameFixesLimit = value;
  });

  geode::listenForSettingChanges("lock_delta", +[](bool value) {
    Global::get().lockDelta = value;
  });

  geode::listenForSettingChanges("auto_stop_playing", +[](bool value) {
    Global::get().stopPlaying = value;
  });

};

class $modify(PlayLayer) {

  struct Fields {
    int delayedLevelRestart = -1;
  };

  void postUpdate(float dt) { 
    PlayLayer::postUpdate(dt);
    auto& g = Global::get();

    if (m_fields->delayedLevelRestart != -1 && m_fields->delayedLevelRestart >= Global::getCurrentFrame()) {
      m_fields->delayedLevelRestart = -1;
      resetLevelFromStart();
    }

  }

  void onQuit() {
    if (Mod::get()->getSettingValue<bool>("disable_speedhack") && Global::get().speedhackEnabled)
      Global::toggleSpeedhack();

    PlayLayer::onQuit();
  }

  void pauseGame(bool b1) {
    Global::updateKeybinds();

    if (!Global::get().renderer.tryPause()) return;

    auto& g = Global::get();

    if (!m_player1 || !m_player2) return PlayLayer::pauseGame(b1);

    if (g.state != state::recording) return PlayLayer::pauseGame(b1);

    g.ignoreRecordAction = true;
    int frame = Global::getCurrentFrame() + 1;

    if (m_player1->m_holdingButtons[1]) {
      handleButton(false, 1, false);
      g.macro.inputs.push_back(input(frame, 1, false, false));
    }
    if (m_levelSettings->m_platformerMode) {
      if (m_player1->m_holdingButtons[2]) {
        handleButton(false, 2, false);
        g.macro.inputs.push_back(input(frame, 2, false, false));
      }
      if (m_player1->m_holdingButtons[3]) {
        handleButton(false, 3, false);
        g.macro.inputs.push_back(input(frame, 3, false, false));
      }
    }

    if (m_levelSettings->m_twoPlayerMode) {
      if (m_player2->m_holdingButtons[1]) {
        handleButton(false, 1, true);
        g.macro.inputs.push_back(input(frame, 1, true, false));
      }
      if (m_levelSettings->m_platformerMode) {
        if (m_player2->m_holdingButtons[2]) {
          handleButton(false, 2, false);
          g.macro.inputs.push_back(input(frame, 2, true, false));
        }
        if (m_player2->m_holdingButtons[3]) {
          handleButton(false, 3, false);
          g.macro.inputs.push_back(input(frame, 3, true, false));
        }
      }
    }

    g.ignoreRecordAction = false;

    PlayLayer::pauseGame(b1);
  }

  bool init(GJGameLevel * level, bool b1, bool b2) {
    auto& g = Global::get();
    g.firstAttempt = true;  

    if (!PlayLayer::init(level, b1, b2)) return false;

    Global::updateKeybinds();

    auto now = std::chrono::system_clock::now();
    g.currentSession = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    g.lastAutoSaveFrame = 0;

    return true;
  }

  void resetLevel() {
    PlayLayer::resetLevel();

    auto& g = Global::get();

    int frame = Global::getCurrentFrame();

    if (!m_isPracticeMode)
      g.renderer.levelStartFrame = frame;

    if (g.restart && m_levelSettings->m_platformerMode && g.state != state::none)
      m_fields->delayedLevelRestart = frame + 2;

    Global::updateSeed(true);

    g.safeMode = false;

    if (g.layoutMode)
      g.safeMode = true;

    g.currentAction = 0;
    g.currentFrameFix = 0;
    g.restart = false;

    if (g.state == state::recording)
      Macro::updateInfo(this);

    if ((!m_isPracticeMode || frame <= 1 || g.checkpoints.empty()) && g.state == state::recording) {
      g.macro.inputs.clear();
      g.macro.frameFixes.clear();
      g.checkpoints.clear();

      g.macro.framerate = 240.f;
      if (g.layer) static_cast<RecordLayer*>(g.layer)->updateTPS();

      PlayerData p1Data = PlayerPracticeFixes::saveData(m_player1);
      PlayerData p2Data = PlayerPracticeFixes::saveData(m_player2);

      InputPracticeFixes::applyFixes(this, p1Data, p2Data, frame);
      Macro::resetVariables();

      m_player1->m_holdingRight = false;
      m_player1->m_holdingLeft = false;
      m_player2->m_holdingRight = false;
      m_player2->m_holdingLeft = false;

      m_player1->m_holdingButtons[2] = false;
      m_player1->m_holdingButtons[3] = false;
      m_player2->m_holdingButtons[2] = false;
      m_player2->m_holdingButtons[3] = false;
    }

    if (!m_levelSettings->m_platformerMode || (!g.mod->getSavedValue<bool>("macro_always_practice_fixes") && g.state != state::recording)) return;

    g.ignoreRecordAction = true;
    for (int i = 0; i < 4; i++) {
      bool player2 = !(sidesButtons[i] > 2);
      bool rightKey = sidesButtons[i] == 5 || sidesButtons[i] == 2;
      if (g.heldButtons[sidesButtons[i]])
        handleButton(true, indexButton[sidesButtons[i]], player2);
    }
    g.ignoreRecordAction = false;
  }

};

class $modify(BGLHook, GJBaseGameLayer) {

  struct Fields {
    bool macroInput = false;
  };

  void processCommands(float dt) {
    auto& g = Global::get();

    PlayLayer* pl = PlayLayer::get();

    if (!pl) {
      // handlePlaying(Global::getCurrentFrame(true));
      // log::debug("{}", Global::getCurrentFrame(true));
      return GJBaseGameLayer::processCommands(dt);
    }

    Global::updateSeed();

    bool rendering = g.renderer.recording || g.renderer.recordingAudio;

    if (g.state != state::none || rendering) {

      if (!g.firstAttempt) {
        g.renderer.dontRender = false;
        g.renderer.dontRecordAudio = false;
      }

      int frame = Global::getCurrentFrame();
      if (frame > 2 && g.firstAttempt && g.macro.xdBotMacro) {
        g.firstAttempt = false;

        if ((m_levelSettings->m_platformerMode || rendering) && !m_levelEndAnimationStarted)
          return pl->resetLevelFromStart();
        else if (!m_levelEndAnimationStarted)
          return pl->resetLevel();
      }

      if (g.previousFrame == frame && frame != 0 && g.macro.xdBotMacro)
        return GJBaseGameLayer::processCommands(dt);

    }

    GJBaseGameLayer::processCommands(dt);

    if (g.state == state::none)
      return;

    int frame = Global::getCurrentFrame();
    g.previousFrame = frame;

    if (g.macro.xdBotMacro && g.restart && !m_levelEndAnimationStarted) {
      if ((m_levelSettings->m_platformerMode && g.state != state::none) || g.renderer.recording || g.renderer.recordingAudio)
        return pl->resetLevelFromStart();
      else
        return pl->resetLevel();
    }

    if (g.state == state::recording)
      handleRecording(frame);

    if (g.state == state::playing)
      handlePlaying(Global::getCurrentFrame());

  }

  void handleRecording(int frame) {
    auto& g = Global::get();

    if (g.ignoreFrame != -1) {
      if (g.ignoreFrame < frame) g.ignoreFrame = -1;
    }

    bool twoPlayers = m_levelSettings->m_twoPlayerMode;

    if (g.delayedFrameInput[0] == frame) {
      g.delayedFrameInput[0] = -1;
      // if ((g.heldButtons[0] && twoPlayers) || (!twoPlayers && (g.heldButtons[0] || g.heldButtons[3])))
        GJBaseGameLayer::handleButton(true, 1, true);
    }

    if (g.delayedFrameInput[1] == frame) {
      g.delayedFrameInput[1] = -1;
      // if ((g.heldButtons[3] && twoPlayers) || (!twoPlayers && (g.heldButtons[0] || g.heldButtons[3])))
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

      if (!m_levelSettings->m_platformerMode)
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

    if (!g.frameFixes || g.macro.inputs.empty()) return;

    if (!g.macro.frameFixes.empty())
      if (1.f / Global::getTPS() * (frame - g.macro.frameFixes.back().frame) < 1.f / g.frameFixesLimit)
        return;
 
    g.macro.recordFrameFix(frame, m_player1, m_player2);

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
      auto input = g.macro.inputs[g.currentAction];

      if (frame != g.respawnFrame) {
        if (Macro::flipControls())
          input.player2 = !input.player2;

        GJBaseGameLayer::handleButton(input.down, input.button, input.player2);
      }

      g.currentAction++;
      g.safeMode = true;
    }

    g.respawnFrame = -1;
    m_fields->macroInput = false;

    if (g.currentAction == g.macro.inputs.size()) {
      if (g.stopPlaying) {
        Macro::togglePlaying();
        Macro::resetState(true);

        return;
      }
    }

    if ((!g.frameFixes && !g.inputFixes) || !PlayLayer::get()) return;

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

    if (g.p2mirror && m_gameState.m_isDualMode && !g.autoclicker) {
      GJBaseGameLayer::handleButton(g.mod->getSavedValue<bool>("p2_input_mirror_inverted") ? !hold : hold, button, !player2);
    }

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

    bool isDelayedInput = g.delayedFrameInput[(m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] != -1;
    bool isDelayedRelease = g.delayedFrameReleaseMain[(m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] != -1;

    if ((isDelayedInput || g.ignoreJumpButton == frame || isDelayedRelease) && button == 1) {
      if (g.ignoreJumpButton >= frame)
        g.delayedFrameInput[(m_levelSettings->m_twoPlayerMode ? static_cast<int>(!player2) : 0)] = g.ignoreJumpButton + 1;

      return;
    }

    if (g.state != state::recording) return GJBaseGameLayer::handleButton(hold, button, player2);

    if (g.inputFixes)
      g.macro.recordFrameFix(frame, m_player1, m_player2);

    GJBaseGameLayer::handleButton(hold, button, player2);

    if (!m_levelSettings->m_twoPlayerMode)
      player2 = false;

    if (!g.ignoreRecordAction && !g.creatingTrajectory && !m_player1->m_isDead) {
      g.macro.recordAction(frame, button, player2, hold);
      if (g.p2mirror && m_gameState.m_isDualMode)
        g.macro.recordAction(frame, button, !player2, g.mod->getSavedValue<bool>("p2_input_mirror_inverted") ? !hold : hold);
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
      if (PlayLayer* pl = PlayLayer::get())
        pl->resetLevel();
    }

  }

  void onQuit(CCObject * sender) {
    PauseLayer::onQuit(sender);

    Macro::resetState();

    Loader::get()->queueInMainThread([] {
      auto& g = Global::get();
      if (g.renderer.recording) g.renderer.stop();
      if (g.renderer.recordingAudio) g.renderer.stopAudio();
    });
  }

  void goEdit() {
    PauseLayer::goEdit();

    Macro::resetState();
    
    Loader::get()->queueInMainThread([] {
      auto& g = Global::get();
      if (g.renderer.recording) g.renderer.stop();
      if (g.renderer.recordingAudio) g.renderer.stopAudio();
    });
  }

};
