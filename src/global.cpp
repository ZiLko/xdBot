#include "ui/record_layer.hpp"
#include "ui/game_ui.hpp"

#include <Geode/modify/CCTextInputNode.hpp>

#ifdef GEODE_IS_WINDOWS
#include <geode.custom-keybinds/include/Keybinds.hpp>
#endif

#include <random>

class $modify(CCTextInputNode) {

    bool ccTouchBegan(cocos2d::CCTouch * v1, cocos2d::CCEvent * v2) {
        if (this->getID() == "disabled-input"_spr) return false;

        return CCTextInputNode::ccTouchBegan(v1, v2);
    }
};

struct IncompatibleSetting {
  std::string ID;
  bool incompatValue;
  bool isModToggle = false;
  bool isSavedValue = false;
};

struct IncompatibleMod {
  std::string ID;
  bool canBeDisabled;
  std::vector<IncompatibleSetting> incompatSettings;
};

const std::vector<IncompatibleMod> incompatibleMods {
  { "syzzi.click_between_frames", true, { {"soft-toggle", false, true }, { "actual-delta", true } } },
  { "alphalaneous.click_after_frames", true, { { "soft-toggle", false, true } } },
  { "thesillydoggo.qolmod", true, { { "tps-bypass_enabled", true, false, true } } },
  // { "zmx.cbf-lite", false, {  } }
};

bool Global::hasIncompatibleMods() {
  std::vector<std::string> modsToDisable;
  std::vector<std::string> settingsToDisable;

  if (Mod* mod = Loader::get()->getLoadedMod("firee.prism")) {
    auto json = mod->getSavedValue<matjson::Value>("values");
    for (const auto& obj : json.asArray().unwrap()) {

      if (obj["name"].asString().unwrapOrDefault() != "TPS Bypass") continue;

      if (obj["value"].asInt().unwrapOrDefault() != 240)
        settingsToDisable.push_back("<cr>TPS Bypass (Prism Menu)</c>");

      break;

    }
  }

  #ifdef GEODE_IS_WINDOWS

  if (Mod* mod = Loader::get()->getLoadedMod("tobyadd.gdh")) {
    std::filesystem::path configPath = mod->getSaveDir() / "config.json";
	  using namespace nlohmann;

    if (std::filesystem::exists(configPath)) {
      std::ifstream jsonFile(configPath);
      if (jsonFile.is_open()) {
        json jsonData;
        jsonFile >> jsonData;
        if (jsonData.contains("tps_enabled")) {
          if (jsonData["tps_enabled"])
            settingsToDisable.push_back("<cr>TPS Bypass (GDH)</c>");
        }
      }
    }
  }

  #else

  if (Mod* mod = Loader::get()->getLoadedMod("tobyadd.gdh_mobile")) {
    std::filesystem::path configPath = mod->getSaveDir() / "config.json";
	  using namespace nlohmann;
    
    if (std::filesystem::exists(configPath)) {
      std::ifstream jsonFile(configPath);
      if (jsonFile.is_open()) {
        json jsonData;
        jsonFile >> jsonData;
        if (jsonData.contains("fps_value")) {
          if (jsonData["fps_value"] != 240)
            settingsToDisable.push_back("<cr>TPS Bypass (GDH)</c>");
        }
      }
    }
  }

  #endif

  for (IncompatibleMod incompatMod : incompatibleMods) {
    Mod* mod = Loader::get()->getLoadedMod(incompatMod.ID);

    if (!mod) continue;

    std::string modName = mod->getName();

    if (!incompatMod.canBeDisabled) {
      modsToDisable.push_back(modName);
      continue;
    }

    for (IncompatibleSetting sett : incompatMod.incompatSettings) {
      bool value = sett.isSavedValue ? mod->getSavedValue<bool>(sett.ID) : mod->getSettingValue<bool>(sett.ID);

      if (value != sett.incompatValue) continue;

      if (sett.isModToggle)
        modsToDisable.push_back(modName);
      else {
        std::string settName = sett.isSavedValue ? sett.ID : mod->getSetting(sett.ID)->getDisplayName();
        settingsToDisable.push_back(fmt::format("{} ({})", settName, modName));
      }

    }
  }

  if (!modsToDisable.empty()) {
    std::string incompatString = "";

    for (const std::string name : modsToDisable)
      incompatString += fmt::format("<cr>{}</c>{}", name, (name != modsToDisable.back() ? ", " : ""));

    FLAlertLayer::create("Warning", "The following mods are incompatible: \n" + incompatString, "Ok")->show();

  } else if (!settingsToDisable.empty()) {
    std::string incompatString = "";

    for (const std::string name : settingsToDisable)
      incompatString += fmt::format("<cr>{}</c>{}", name, (name != settingsToDisable.back() ? ", " : ""));

    FLAlertLayer::create("Warning", "The following mod settings are incompatible: \n" + incompatString, "Ok")->show();
    
  }

  bool ret = !modsToDisable.empty() || !settingsToDisable.empty();

  if (ret) {
    Global::get().state = state::none;
    Interface::updateLabels();
    Interface::updateButtons();
  }

  return ret;
}

int Global::getTPS() {
  auto& g = Global::get();
  return g.tpsEnabled ? g.tps : 240;
}

int Global::getCurrentFrame(bool editor) {
  double levelTime;
  PlayLayer* pl = PlayLayer::get();

  if (!pl) {
    if (!editor) return 0;

    levelTime = GJBaseGameLayer::get()->m_gameState.m_levelTime;
  }

  levelTime = pl->m_gameState.m_levelTime;
  int frame = static_cast<int>(levelTime * getTPS());

  frame -= Global::get().frameOffset;
  if (frame < 0) return 0;

  return frame;
}

void Global::updateKeybinds() {
#ifdef GEODE_IS_WINDOWS

  auto& g = Global::get();
  for (size_t i = 0; i < 6; i++) {
    auto keys = keybinds::BindManager::get()->getBindsFor(buttonIDs[i]);
    std::vector<int> keysInts = {};

    for (size_t j = 0; j < keys.size(); j++) {
      keysInts.push_back(keys[j]->getHash());
      g.allKeybinds.insert(keys[j]->getHash());
    }

    g.keybinds[i].clear();
    for (int k = 0; k < keysInts.size(); k++)
      g.keybinds[i].push_back(keysInts[k]);
  }
#endif
}

void Global::updateSeed(bool isRestart) {

  auto& g = Global::get();

  if (g.seedEnabled) {
    PlayLayer* pl = PlayLayer::get();
    if (!pl) return;

    unsigned long long ull = std::stoull(g.mod->getSavedValue<std::string>("macro_seed"), nullptr, 0);
    uintptr_t seed = static_cast<uintptr_t>(ull);
    int finalSeed;

    if (!pl->m_player1->m_isDead) {
      std::mt19937 generator(seed + pl->m_gameState.m_currentProgress);
      std::uniform_int_distribution<int> distribution(100000, 999999999);
      int randomSeed = distribution(generator);
    }
    else {
      std::random_device rd;
      std::mt19937 generator(rd());
      std::uniform_int_distribution<int> distribution(1000, 999999999);
      finalSeed = distribution(generator);
    }

#ifdef GEODE_IS_WINDOWS
    *(uintptr_t*)((char*)geode::base::get() + seedAddr) = finalSeed;
#else
    GameToolbox::fast_srand(finalSeed);
#endif

    g.safeMode = true;
  }

  if (isRestart && g.state == state::recording) {
#ifdef GEODE_IS_WINDOWS
    g.macro.seed = *(uintptr_t*)((char*)geode::base::get() + seedAddr);
#else
    g.macro.seed = 0;
#endif
  }

}

void Global::updatePitch(float value) {
  auto& g = Global::get();
  if (!g.speedhackAudio) {
    if (g.currentPitch != 1.f) value = 1.f;
    else return;
  }

  FMODAudioEngine* fmod = FMODAudioEngine::sharedEngine();
  FMOD::ChannelGroup* channel = nullptr;
  fmod->m_system->getMasterChannelGroup(&channel);

  if (channel) {
    channel->setPitch(value);
    g.currentPitch = value;
  }
}

void Global::frameStep() {
  auto& g = Global::get();
  if (!PlayLayer::get() || !g.frameStepper) return;

  g.stepFrame = true;
  g.stepFrameDraw = true;
  g.stepFrameParticle = 4;
}

void Global::toggleSpeedhack() {
  auto& g = Global::get();
  g.mod->setSavedValue("macro_speedhack_enabled", !g.mod->getSavedValue<bool>("macro_speedhack_enabled"));
  g.speedhackEnabled = g.mod->getSavedValue<bool>("macro_speedhack_enabled");

  if (g.layer) {
    if (static_cast<RecordLayer*>(g.layer)->speedhackToggle)
      static_cast<RecordLayer*>(g.layer)->speedhackToggle->toggle(g.mod->getSavedValue<bool>("macro_speedhack_enabled"));
  }

  if (!g.mod->getSavedValue<bool>("macro_speedhack_enabled"))
    Global::updatePitch(1.f);
}

void Global::toggleFrameStepper() {
  if (Global::get().frameStepper)
    Global::frameStepperOff();
  else
    Global::frameStepperOn();
}

void Global::frameStepperOn() {
  auto& g = Global::get();

  g.mod->setSavedValue("macro_frame_stepper", true);
  g.frameStepper = true;

  if (g.layer) {
    if (static_cast<RecordLayer*>(g.layer)->frameStepperToggle)
      static_cast<RecordLayer*>(g.layer)->frameStepperToggle->toggle(true);
  }

  if (PlayLayer::get())
    g.frameStepperMusicTime = FMODAudioEngine::sharedEngine()->getMusicTimeMS(0);

  Interface::updateButtons();
}

void Global::frameStepperOff() {
  auto& g = Global::get();

  g.mod->setSavedValue("macro_frame_stepper", false);
  g.stepFrame = false;
  g.stepFrameParticle = false;
  g.frameStepper = false;

  if (PlayLayer::get() && g.frameStepperMusicTime != 0) {
    FMODAudioEngine::sharedEngine()->setMusicTimeMS(g.frameStepperMusicTime, true, 0);
    g.frameStepperMusicTime = 0;
  }

  if (g.layer) {
    if (static_cast<RecordLayer*>(g.layer)->frameStepperToggle)
      static_cast<RecordLayer*>(g.layer)->frameStepperToggle->toggle(false);
  }

  Interface::updateButtons();
}

PauseLayer* Global::getPauseLayer() {
  CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
  CCObject* child;
  CCARRAY_FOREACH(children, child) {
    if (PauseLayer* pauseLayer = typeinfo_cast<PauseLayer*>(child))
      return pauseLayer;
  }

  return nullptr;
}

$execute{
  auto & g = Global::get();

  if (!g.mod->setSavedValue("defaults_set_12", true)) {
    g.mod->setSettingValue<std::filesystem::path>("macros_folder", g.mod->getSaveDir() / "macros");
    g.mod->setSettingValue<std::filesystem::path>("autosaves_folder", g.mod->getSaveDir() / "autosaves");
  }

  #ifdef GEODE_IS_ANDROID
  if (!g.mod->setSavedValue("defaults_set_11", true))
    g.mod->setSavedValue("render_codec", std::string("libx264"));
  #endif

  if (!g.mod->setSavedValue("defaults_set_10", true)) {
    g.mod->setSettingValue("restore_page", true);

    g.mod->setSavedValue("autosave_interval_enabled", false);
    g.mod->setSavedValue("autosave_interval", std::to_string(10));
    g.mod->setSavedValue("autosave_checkpoint_enabled", true);
    g.mod->setSavedValue("autosave_levelend_enabled", true);
    
    g.mod->setSavedValue("render_fade_in_video", std::to_string(2));
    g.mod->setSavedValue("render_fade_out_video", std::to_string(2));

    g.mod->setSavedValue("macro_auto_stop_playing", false);
    g.mod->setSavedValue("macro_tps", 240.f);
    g.mod->setSavedValue("macro_tps_enabled", false);

    g.mod->setSavedValue("autoclicker_hold_for", 5);
    g.mod->setSavedValue("autoclicker_release_for", 5);
    g.mod->setSavedValue("autoclicker_hold_for2", 5);
    g.mod->setSavedValue("autoclicker_release_for2", 5);
    g.mod->setSavedValue("autoclicker_p1", true);
    g.mod->setSavedValue("autoclicker_p2", true);

    g.mod->setSavedValue("trajectory_color1", ccc3(74, 226, 85));
    g.mod->setSavedValue("trajectory_color2", ccc3(130, 8, 8));
    g.mod->setSavedValue("trajectory_length", std::to_string(240));

  }

  if (!g.mod->setSavedValue("defaults_set3", true)) {
    g.mod->setSettingValue<std::filesystem::path>("render_folder", g.mod->getSaveDir() / "renders");
    g.mod->setSavedValue("render_file_extension", std::string(".mp4"));
    g.mod->setSavedValue("render_sfx_volume", 1.f);
    g.mod->setSavedValue("render_music_volume", 1.f);
    g.mod->setSavedValue("respawn_time", 0.5f);
    g.mod->setSavedValue("render_seconds_after", std::to_string(2));
    g.mod->setSavedValue("render_record_audio", true);
    g.mod->setSavedValue("render_args", std::string("-pix_fmt yuv420p"));
    g.mod->setSavedValue("macro_noclip_p1", true);
    g.mod->setSavedValue("macro_noclip_p2", true);

    g.mod->setSavedValue("render_width2", std::to_string(1920));
    g.mod->setSavedValue("render_height", std::to_string(1080));
    g.mod->setSavedValue("render_bitrate", std::to_string(12));
    g.mod->setSavedValue("render_fps", std::to_string(60));
    g.mod->setSavedValue("render_video_args", std::string("colorspace=all=bt709:iall=bt470bg:fast=1"));

    g.mod->setSavedValue("render_codec", std::string("libx264"));
    #ifdef GEODE_IS_WINDOWS
    g.mod->setSettingValue("ffmpeg_path", geode::dirs::getGameDir() / "ffmpeg.exe");
    #endif

    g.mod->setSavedValue("render_record_audio", true);
    g.mod->setSavedValue("render_hide_labels", true);

    g.mod->setSavedValue("macro_seed", std::to_string(1));
    g.mod->setSavedValue("macro_speedhack", std::string("0.5"));
    g.mod->setSavedValue("macro_fps", 3);

    g.mod->setSavedValue("macro_ignore_inputs", true);
    g.mod->setSavedValue("macro_auto_safe_mode", true);
    g.mod->setSavedValue("macro_speedhack_audio", true);
    g.mod->setSavedValue("macro_show_frame_label", false);
    g.mod->setSavedValue("macro_hide_playing_label", true);

    g.mod->setSavedValue("menu_show_button", true);
    g.mod->setSavedValue("menu_pause_on_open", false);
    g.mod->setSavedValue("menu_show_cursor", true);

    #ifdef GEODE_IS_ANDROID
    g.mod->setSavedValue("menu_show_cursor", false);
    #endif

  }

  g.showTrajectory = g.mod->getSavedValue<bool>("macro_show_trajectory");
  g.coinFinder = g.mod->getSavedValue<bool>("macro_coin_finder");
  g.frameStepper = g.mod->getSavedValue<bool>("macro_frame_stepper");
  g.seedEnabled = g.mod->getSavedValue<bool>("macro_seed_enabled");
  g.frameLabel = g.mod->getSavedValue<bool>("macro_show_frame_label");
  g.speedhackAudio = g.mod->getSavedValue<bool>("macro_speedhack_audio");
  g.trajectoryBothSides = g.mod->getSavedValue<bool>("macro_trajectory_both_sides");
  g.p2mirror = g.mod->getSavedValue<bool>("p2_input_mirror");
  g.tpsEnabled = g.mod->getSavedValue<bool>("macro_tps_enabled");
  g.tps = g.mod->getSavedValue<double>("macro_tps");
  g.autoclicker = g.mod->getSavedValue<bool>("autoclicker_enabled");
  g.autoclickerP1 = g.mod->getSavedValue<bool>("autoclicker_p1");
  g.autoclickerP2 = g.mod->getSavedValue<bool>("autoclicker_p2");
  g.disableShaders = g.mod->getSavedValue<bool>("disableShaders");
  g.autosaveIntervalEnabled = g.mod->getSavedValue<bool>("autosave_interval_enabled");
  g.autosaveEnabled = g.mod->getSavedValue<bool>("macro_auto_save");

  g.holdFor = g.mod->getSavedValue<int64_t>("autoclicker_hold_for");
  g.releaseFor = g.mod->getSavedValue<int64_t>("autoclicker_release_for");
  g.holdFor2 = g.mod->getSavedValue<int64_t>("autoclicker_hold_for2");
  g.releaseFor2 = g.mod->getSavedValue<int64_t>("autoclicker_release_for2");
  g.currentPage = g.mod->getSavedValue<int64_t>("current_page");

  g.autosaveInterval = (geode::utils::numFromString<float>(g.mod->getSavedValue<std::string>("autosave_interval")).unwrapOr(0.f) * 60);
  
  g.speedhackEnabled = false;
  g.mod->setSavedValue("macro_speedhack_enabled", false);

  g.frameOffset = g.mod->getSettingValue<int64_t>("frame_offset");
  g.frameFixesLimit = g.mod->getSettingValue<int64_t>("frame_fixes_limit");
  g.lockDelta = g.mod->getSettingValue<bool>("lock_delta");
  g.stopPlaying = g.mod->getSettingValue<bool>("auto_stop_playing");

  if (g.mod->getSettingValue<std::string>("macro_accuracy") == "Frame Fixes")
    g.frameFixes = true;
  else if (g.mod->getSettingValue<std::string>("macro_accuracy") == "Input Fixes")
    g.inputFixes = true;

  g.macro.author = "N/A";
  g.macro.description = "N/A";
  g.macro.gameVersion = 2.206;
};
