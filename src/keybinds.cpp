
#include "includes.hpp"
#include "ui/record_layer.hpp"
#include "ui/game_ui.hpp"
#include "ui/clickbot_layer.hpp"
#include "ui/macro_editor.hpp"
#include "ui/render_settings_layer.hpp"
#include "hacks/layout_mode.hpp"

#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>

#ifdef GEODE_IS_WINDOWS

#include <geode.custom-keybinds/include/Keybinds.hpp>
#include <regex>

#endif

const std::vector<std::string> keybindIDs = {
    "open_menu", "toggle_recording", "toggle_playing",
    "toggle_speedhack", "toggle_frame_stepper", "step_frame",
    "toggle_render", "toggle_noclip", "show_trajectory"
};

class $modify(CCKeyboardDispatcher) {
  bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat) {
  
    auto& g = Global::get();

    int keyInt = static_cast<int>(key);
    if (g.allKeybinds.contains(keyInt) && !isKeyRepeat) {
      for (size_t i = 0; i < 6; i++) {
        if (std::find(g.keybinds[i].begin(), g.keybinds[i].end(), keyInt) != g.keybinds[i].end())
          g.heldButtons[i] = isKeyDown;
      }
    }

    if (key == enumKeyCodes::KEY_F && !isKeyRepeat && isKeyDown && PlayLayer::get()) {
      log::debug("POS DEBUG {}", PlayLayer::get()->m_player1->getPosition());
      log::debug("POS2 DEBUG {}", PlayLayer::get()->m_player2->getPosition());
    }


    if (key == enumKeyCodes::KEY_J && !isKeyRepeat && isKeyDown) {
      //  std::string str = ZipUtils::decompressString(PlayLayer::get()->m_level->m_levelString.c_str(), true, 0);
    // auto strs = Utils::splitByChar(str, ';');

    //   auto start = std::chrono::high_resolution_clock::now();

    // std::string lole = LayoutMode::mergeVector(strs, ";");

    // auto endd = std::chrono::high_resolution_clock::now();

    // std::chrono::duration<double, std::milli> elapsed = endd - start;

    // log::debug("{}", str);
    
    }
     
      // MacroEditLayer::create()->show();
       
    // if (key == enumKeyCodes::KEY_J && !isKeyRepeat && isKeyDown) {
      // CCKeyboardDispatcher::get()->dispatchKeyboardMSG(enumKeyCodes::KEY_Space, true, false);
      // CCKeyboardDispatcher::get()->dispatchKeyboardMSG(enumKeyCodes::KEY_Space, false, false);
      // auto layer = static_cast<EndLevelLayer*>(PlayLayer::get()->getChildByID("EndLevelLayer"));
      // layer->onReplay(static_cast<CCObject*>(layer->m_mainLayer->getChildByID("button-menu")->getChildByID("retry-button")));

    // }
    /*if (key == enumKeyCodes::KEY_L && !isKeyRepeat && isKeyDown) {

    }*/


    return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat);
  }
};

#ifdef GEODE_IS_ANDROID

namespace keybinds {

  struct ActionID {};

};

#endif

using namespace keybinds;

void onKeybind(bool down, ActionID id) {
#ifdef GEODE_IS_WINDOWS

  auto& g = Global::get();

  if (!down || (LevelEditorLayer::get() && !g.mod->getSettingValue<bool>("editor_keybinds")) || g.mod->getSettingValue<bool>("disable_keybinds"))
    return;

  if (id == "open_menu"_spr) {
    if (g.layer) {
      static_cast<RecordLayer*>(g.layer)->onClose(nullptr);
      return;
    }

    RecordLayer::openMenu();
  }

  if (id == "toggle_recording"_spr)
    Macro::toggleRecording();

  if (id == "toggle_playing"_spr)
    Macro::togglePlaying();

  if (id == "toggle_frame_stepper"_spr && PlayLayer::get())
    Global::toggleFrameStepper();

  if (id == "step_frame"_spr)
    Global::frameStep();

  if (id == "toggle_speedhack"_spr)
    Global::toggleSpeedhack();

  if (id == "show_trajectory"_spr) {
    g.mod->setSavedValue("macro_show_trajectory", !g.mod->getSavedValue<bool>("macro_show_trajectory"));

    if (g.layer) {
      if (static_cast<RecordLayer*>(g.layer)->trajectoryToggle)
        static_cast<RecordLayer*>(g.layer)->trajectoryToggle->toggle(g.mod->getSavedValue<bool>("macro_show_trajectory"));
    }

    g.showTrajectory = g.mod->getSavedValue<bool>("macro_show_trajectory");
  }

  if (id == "toggle_render"_spr && PlayLayer::get()) {
    bool result = Renderer::toggle();

    if (result && Global::get().renderer.recording)
      Notification::create("Started Rendering", NotificationIcon::Info)->show();

    if (g.layer) {
      if (static_cast<RecordLayer*>(g.layer)->renderToggle)
        static_cast<RecordLayer*>(g.layer)->renderToggle->toggle(Global::get().renderer.recording);
    }

  }

  if (id == "toggle_noclip"_spr) {
    g.mod->setSavedValue("macro_noclip", !g.mod->getSavedValue<bool>("macro_noclip"));

    if (g.layer) {
      if (static_cast<RecordLayer*>(g.layer)->noclipToggle)
        static_cast<RecordLayer*>(g.layer)->noclipToggle->toggle(g.mod->getSavedValue<bool>("macro_noclip"));
    }
  }

#endif

}

$execute{

  #ifdef GEODE_IS_WINDOWS

    BindManager * bm = BindManager::get();

    bm->registerBindable({
        "open_menu"_spr,
        "Open Menu",
        "Open Menu.",
        { Keybind::create(KEY_F, Modifier::Alt) },
        "xdBot",
        false
    });

    bm->registerBindable({
        "toggle_recording"_spr,
        "Record macro",
        "Toggles recording.",
        { Keybind::create(KEY_G, Modifier::Alt) },
        "xdBot",
        false
    });

    bm->registerBindable({
      "toggle_playing"_spr,
      "Play macro",
      "Toggles playing.",
      { Keybind::create(KEY_H, Modifier::Alt) },
      "xdBot",
        false
    });

    bm->registerBindable({
      "toggle_speedhack"_spr,
      "Speedhack",
      "Toggles speedhack.",
      { Keybind::create(KEY_S, Modifier::Alt) },
      "xdBot",
        false
    });

    bm->registerBindable({
      "toggle_noclip"_spr,
      "NoClip",
      "Toggles NoClip.",
      { Keybind::create(KEY_N, Modifier::Alt) },
      "xdBot",
        false
    });

    bm->registerBindable({
      "toggle_frame_stepper"_spr,
      "Toggle Frame Stepper",
      "Toggles frame stepper..",
      { Keybind::create(KEY_C, Modifier::Alt) },
      "xdBot",
      false
    });

    bm->registerBindable({
      "step_frame"_spr,
      "Advance frame",
      "Advances one frame if frame stepper is on.",
      { Keybind::create(KEY_V) },
      "xdBot"
    });

    bm->setRepeatOptionsFor("step_frame"_spr, { true, 10, 450 });

    bm->registerBindable({
      "show_trajectory"_spr,
      "Show Trajectory",
      "Toggles Show Trajectory.",
      { Keybind::create(KEY_T, Modifier::Alt) },
      "xdBot"
    });

    bm->registerBindable({
      "toggle_render"_spr,
      "Render",
      "Toggles rendering.",
      { Keybind::create(KEY_P, Modifier::Alt) },
      "xdBot",
      false
    });

    for (int i = 0; i < keybindIDs.size(); i++) {
        new EventListener([=](InvokeBindEvent* event) { onKeybind(event->isDown(), event->getID()); return ListenerResult::Propagate;
        }, InvokeBindFilter(nullptr, (""_spr) + keybindIDs[i]));
    }

  #endif
}
