#include "ui/record_layer.hpp"
#include "ui/game_ui.hpp"

#include "ui/button_setting.hpp"

#include <Geode/loader/SettingNode.hpp>

#ifdef GEODE_IS_WINDOWS

#include <geode.custom-keybinds/include/Keybinds.hpp>

#endif

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
    for (const auto& obj : json.as_array()) {

      if (obj["name"] != "TPS Bypass") continue;

      if (obj["value"] != 240)
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
        std::string settName = sett.isSavedValue ? sett.ID : mod->getSettingDefinition(sett.ID)->getDisplayName();
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

int Global::getCurrentFrame() {
  PlayLayer* pl = PlayLayer::get();
  if (!pl) return 0;

  auto& g = Global::get();

  int frame = static_cast<int>(pl->m_gameState.m_levelTime * 240.0);

  frame -= g.frameOffset;

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
    unsigned long long ull = std::stoull(g.mod->getSavedValue<std::string>("macro_seed"), nullptr, 0);
    uintptr_t seed = static_cast<uintptr_t>(ull);

#ifdef GEODE_IS_WINDOWS
    * (uintptr_t*)((char*)geode::base::get() + seedAddr) = seed;
#else
    GameToolbox::fast_srand(seed);
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

SettingNode* ButtonSettingValue::createNode(float width) {
  return ButtonSettingNode::create(this, width);
}

$on_mod(Loaded) {
  auto& g = Global::get();
  g.mod = Mod::get();

  g.mod->addCustomSetting<ButtonSettingValue>("button", "none");
}

$execute{
    auto & g = Global::get();

  if (!g.mod->setSavedValue("defaults_set5", true)) {
    g.mod->setSettingValue<std::filesystem::path>("render_folder", g.mod->getSaveDir() / "renders");
    g.mod->setSavedValue("macro_hide_playing_label", true);
  }

  if (!g.mod->setSavedValue("defaults_set4", true)) {
    g.mod->setSavedValue("macro_noclip_p1", true);
    g.mod->setSavedValue("macro_noclip_p2", true);
  }

  if (!g.mod->setSavedValue("defaults_set6", true))
    g.mod->setSavedValue("render_args", std::string("-pix_fmt yuv420p"));

  if (!g.mod->setSavedValue("defaults_set7", true)) {
    g.mod->setSavedValue("render_seconds_after", std::to_string(2));
    g.mod->setSavedValue("render_record_audio", true);
  }

  if (!g.mod->setSavedValue("defaults_set3", true)) {
    g.mod->setSavedValue("render_width2", std::to_string(1920));
    g.mod->setSavedValue("render_height", std::to_string(1080));
    g.mod->setSavedValue("render_bitrate", std::to_string(12));
    g.mod->setSavedValue("render_fps", std::to_string(60));
    g.mod->setSavedValue("render_video_args", std::string("colorspace=all=bt709:iall=bt470bg:fast=1"));

    #ifdef GEODE_IS_WINDOWS
    g.mod->setSavedValue("render_codec", std::string("h264"));
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
    g.mod->setSavedValue("macro_auto_stop_playing", true);
    g.mod->setSavedValue("macro_show_frame_label", true);

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
  g.frameFixes = g.mod->getSettingValue<bool>("frame_fixes");

  g.speedhackEnabled = false;
  g.mod->setSavedValue("macro_speedhack_enabled", false);

  g.frameOffset = g.mod->getSettingValue<int64_t>("frame_offset");

  g.macro.author = "N/A";
  g.macro.description = "N/A";
  g.macro.gameVersion = 2.206;
};
