#include "record_layer.hpp"
#include "macro_editor.hpp"
#include "game_ui.hpp"
#include "render_presets_layer.hpp"
#include "clickbot_layer.hpp"
#include "noclip_settings_layer.hpp"
#include "trajectory_settings_layer.hpp"

#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/utils/web.hpp>

const std::vector<std::vector<RecordSetting>> settings {
	{
		{ "Speedhack:", "macro_speedhack_enabled", InputType::Speedhack, 0.4f },
		{ "Seed:", "macro_seed_enabled", InputType::Seed, 0.4f },
		{ "Show Trajectory:", "macro_show_trajectory", InputType::Settings, 0.325f, menu_selector(TrajectorySettingsLayer::open)  },
		{ "Enable Coin Finder:", "macro_coin_finder", InputType::None },
		{ "Enable Noclip:", "macro_noclip", InputType::Settings, 0.325f, menu_selector(NoclipSettingsLayer::open) },
		{ "Enable Frame Stepper:", "macro_frame_stepper", InputType::None }
	},
	{
		{ "Enable Auto Saving:", "macro_auto_save", InputType::Autosave },
		{ "Enable Layout Mode:", "macro_layout_mode", InputType::None },
		{ "Auto Safe Mode:", "macro_auto_safe_mode", InputType::None },
		{ "Instant respawn:", "macro_instant_respawn", InputType::None },
		{ "No death effect:", "macro_no_death_effect", InputType::None },
		{ "No respawn flash:", "macro_no_respawn_flash", InputType::None }
	},
	{
	#ifdef GEODE_IS_WINDOWS
		{ "Force cursor on open:", "menu_show_cursor", InputType::None },
		{ "Button on pause menu:", "menu_show_button", InputType::None },
		{ "Pause on open:", "menu_pause_on_open", InputType::None },
	#else
		{ "Always show buttons:", "macro_always_show_buttons", InputType::None },
		{ "Hide speedhack button:", "macro_hide_speedhack", InputType::None },
		{ "Hide Frame Stepper button:", "macro_hide_stepper", InputType::None, 0.3f },
	#endif
		{ "Hide labels on render:", "render_hide_labels", InputType::None },
		{ "Hide playing label:", "macro_hide_playing_label", InputType::None },
		{ "Hide recording label:", "macro_hide_recording_label", InputType::None }
	},
	{
		{ "Enable Clickbot:", "clickbot_enabled", InputType::Settings, 0.325f, menu_selector(ClickbotLayer::open)},
		{ "Speedhack Audio:", "macro_speedhack_audio", InputType::None },
		{ "Always Practice Fixes:", "macro_always_practice_fixes", InputType::None },
		{ "Show Frame Label:", "macro_show_frame_label", InputType::None },
		{ "Ignore inputs:", "macro_ignore_inputs", InputType::None },
		{ "Auto Stop Playing:", "macro_auto_stop_playing", InputType::None }
	},
    {
		{ "Respawn Time:", "respawn_time_enabled", InputType::Respawn },
		{ "Input Mirror:", "p2_input_mirror", InputType::None },
		{ "Disable Shaders:", "disable_shaders", InputType::None },
		{ "Instant Mirror Portal:", "instant_mirror_portal", InputType::None },
		{ "No Mirror Portal:", "no_mirror_portal", InputType::None },
		{ "Lock Delta:", "macro_lock_delta", InputType::None }
    }
};

class $modify(PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        #ifdef GEODE_IS_WINDOWS

        if (!Global::get().mod->getSavedValue<bool>("menu_show_button")) return;

        #endif

        CCSprite* sprite = nullptr;

        sprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
        sprite->setScale(0.35f);


        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(sprite,
            this,
            menu_selector(RecordLayer::openMenu2));
        
        if (!Loader::get()->isModLoaded("geode.node-ids")) {
            CCMenu* menu = CCMenu::create();
            menu->setID("button"_spr);
            addChild(menu);
            btn->setPosition({214, 88});
            menu->addChild(btn);
            return;
        }

        CCNode* menu = this->getChildByID("right-button-menu");
        menu->addChild(btn);
        menu->updateLayout();
    }
};

$execute{
    geode::listenForSettingChanges("frame_offset", +[](int64_t value) {
        auto& g = Global::get();
        g.frameOffset = value;

        if (g.layer) {
            static_cast<RecordLayer*>(g.layer)->warningLabel->setString(("WARNING: Currently recording / playing macros with a frame offset of " + std::to_string(value)).c_str());
            static_cast<RecordLayer*>(g.layer)->warningLabel->setVisible(value != 0);
            static_cast<RecordLayer*>(g.layer)->warningSprite->setVisible(value != 0);
        }

  });

    geode::listenForSettingChanges("background_color", +[](cocos2d::ccColor3B value) {
        auto& g = Global::get();
        if (g.layer) {
            CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
            if (FLAlertLayer* layer = typeinfo_cast<FLAlertLayer*>(children->lastObject()))
                layer->keyBackClicked();

            static_cast<RecordLayer*>(g.layer)->onClose(nullptr);
            RecordLayer::openMenu(true);
        }
  });
};

class $modify(CCTextInputNode) {

    bool ccTouchBegan(cocos2d::CCTouch * v1, cocos2d::CCEvent * v2) {
        if (this->getID() == "android-disabled") return false;

        return CCTextInputNode::ccTouchBegan(v1, v2);
    }
};

void RecordLayer::openSaveMacro(CCObject*) {
    SaveMacroLayer::open();
}

void RecordLayer::openLoadMacro(CCObject*) {
    LoadMacroLayer::open(static_cast<geode::Popup<>*>(this), nullptr);
}

RecordLayer* RecordLayer::openMenu(bool instant) {
    auto& g = Global::get();
    PlayLayer* pl = PlayLayer::get();
    bool cursor = false;

    CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
    CCObject* child;

    if (g.layer)
        static_cast<RecordLayer*>(g.layer)->onClose(nullptr);

    if (pl && g.mod->getSavedValue<bool>("menu_pause_on_open")) {
        if (!pl->m_isPaused)
            pl->pauseGame(false);
    }
#ifdef GEODE_IS_WINDOWS
    else if (pl && g.mod->getSavedValue<bool>("menu_show_cursor")) {
        cursor = cocos2d::CCEGLView::sharedOpenGLView()->getShouldHideCursor();
        cocos2d::CCEGLView::sharedOpenGLView()->showCursor(true);
    }
#endif

    RecordLayer* layer = create();
    layer->cursorWasHidden = cursor;
    layer->m_noElasticity = instant || Global::get().speedhackEnabled;
    layer->show();

    g.layer = static_cast<geode::Popup<>*>(layer);

        if (Loader::get()->isModLoaded("spaghettdev.betterinputs") && !instant) {
            if (!g.mod->setSavedValue("betterinputs-warning", true))
                FLAlertLayer::create("Warning", "<cr>BetterInputs</c> might cause <cr>undefined behavior</c> in xdBot's text inputs.", "Ok")->show();
        }

    return layer;
}

void RecordLayer::checkSpeedhack() {
    std::string speedhackValue = mod->getSavedValue<std::string>("macro_speedhack");

    if (std::count(speedhackValue.begin(), speedhackValue.end(), '.') == 0)
        speedhackValue += ".0";

    if (speedhackValue.back() == '.')
        speedhackValue += "0";

    if (speedhackValue[0] == '0' && speedhackValue[1] != '.')
        speedhackValue.erase(0, 1);

    if (speedhackValue[0] == '.')
        speedhackValue = "0" + speedhackValue;

    mod->setSavedValue("macro_speedhack", speedhackValue);
}

void RecordLayer::onClose(CCObject*) {
    checkSpeedhack();

    PlayLayer* pl = PlayLayer::get();

    if (cursorWasHidden && pl)
        PlatformToolbox::hideCursor();

    Global::get().layer = nullptr;

    this->setKeypadEnabled(false);
    this->setTouchEnabled(false);
    this->removeFromParentAndCleanup(true);
}

void RecordLayer::toggleRecording(CCObject*) {
    auto& g = Global::get();

    if (Global::hasIncompatibleMods())
        return recording->toggle(true);

    if (g.state == state::playing) playing->toggle(false);
    g.state = g.state == state::recording ? state::none : state::recording;

    if (g.state == state::recording) {
        g.currentAction = 0;
        g.currentFrameFix = 0;
        g.restart = true;

        PlayLayer* pl = PlayLayer::get();
        if (pl) {
            if (!pl->m_isPaused)
                pl->pauseGame(false);
        }
    }

    Interface::updateLabels();
    Interface::updateButtons();
}

void RecordLayer::togglePlaying(CCObject*) {
    auto& g = Global::get();

    if (Global::hasIncompatibleMods())
        return playing->toggle(true);

    if (g.state == state::recording)
        recording->toggle(false);

    g.state = g.state == state::playing ? state::none : state::playing;

    if (g.state == state::playing) {
        g.currentAction = 0;
        g.currentFrameFix = 0;

        PlayLayer* pl = PlayLayer::get();

        if (pl) {
            if (!pl->m_isPaused && !pl->m_levelEndAnimationStarted)
                pl->m_levelSettings->m_platformerMode ? pl->resetLevelFromStart() : pl->resetLevel();
            else
                g.restart = true;
        }
    }

    Interface::updateLabels();
    Interface::updateButtons();

}

void RecordLayer::toggleRender(CCObject* btn) {
    if (!Renderer::toggle())
        static_cast<CCMenuItemToggler*>(btn)->toggle(true);

    if (Global::get().renderer.recordingAudio)
        static_cast<CCMenuItemToggler*>(btn)->toggle(false);
}

void RecordLayer::onEditMacro(CCObject*) {
    MacroEditLayer::open();
}

void RecordLayer::toggleFPS(bool on) { // forgotten
    return;
    float scaleSpr = -0.8, scaleBtn = -1;
    int opacityBtn = 57, opacityLbl = 80;

    if (on) {
        return;
        scaleSpr = 0.8;
        scaleBtn = 1;
        opacityBtn = 230;
        opacityLbl = 255;
    }

    CCSprite* spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    spr->setScale(scaleSpr);
    FPSLeft->setSprite(spr);
    FPSLeft->setScale(scaleBtn);
    FPSLeft->setOpacity(opacityBtn);

    spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    spr->setScale(scaleSpr);
    FPSRight->setSprite(spr);
    FPSRight->setScale(scaleBtn);
    FPSRight->setOpacity(opacityBtn);

    fpsLabel->setOpacity(opacityLbl);
}

void RecordLayer::macroInfo(CCObject*) {
    MacroInfoLayer::create()->show();
}

void RecordLayer::textChanged(CCTextInputNode* node) {
    if (!node) return;

    mod = Mod::get();

    if (node == seedInput) {

        try {
            std::string value = seedInput->getString();
            if (value != "") {
                unsigned long long ull = std::stoull(value, nullptr, 0);
                uintptr_t intPtr = static_cast<uintptr_t>(ull);

                mod->setSavedValue("macro_seed", value);
            }

            return;
        }
        catch (const std::invalid_argument& e) {
            return seedInput->setString(mod->getSavedValue<std::string>("macro_seed").c_str());
        }
        catch (const std::out_of_range& e) {
            return seedInput->setString(mod->getSavedValue<std::string>("macro_seed").c_str());
        }

    }

    mod->setSavedValue("render_codec", std::string(codecInput->getString()));

    if (std::string_view(widthInput->getString()) != "")
        mod->setSavedValue("render_width2", std::string(widthInput->getString()));

    if (std::string_view(heightInput->getString()) != "")
        mod->setSavedValue("render_height", std::string(heightInput->getString()));

    if (std::string_view(bitrateInput->getString()) != "")
        mod->setSavedValue("render_bitrate", std::string(bitrateInput->getString()));

    if (std::string_view(fpsInput->getString()) != "" && node == fpsInput) {
        if (std::stoi(fpsInput->getString()) > 240)
            return fpsInput->setString(mod->getSavedValue<std::string>("render_fps").c_str());
        mod->setSavedValue("render_fps", std::string(fpsInput->getString()));
    }

    if (respawnInput) {
        std::string str = respawnInput->getString();
        mod->setSavedValue("respawn_time", numFromString<double>(str).unwrapOr(0.5));
    }

    if (!speedhackInput) return;

    if (std::string_view(speedhackInput->getString()) != "" && node == speedhackInput) {
        std::string value = speedhackInput->getString();

        if (value == ".")
            speedhackInput->setString("0.");
        else if (std::count(value.begin(), value.end(), '.') == 2 || std::stof(value) > 10)
            return speedhackInput->setString(mod->getSavedValue<std::string>("macro_speedhack").c_str());
    }

    if (std::string_view(speedhackInput->getString()) != "")
        mod->setSavedValue("macro_speedhack", std::string(speedhackInput->getString()));
}

void RecordLayer::updatePage(CCObject* obj) {
    auto& g = Global::get();
    g.currentPage += static_cast<CCNode*>(obj)->getID() == "page-left" ? -1 : 1;
    if (g.currentPage == -1) g.currentPage = settings.size() - 1;
    else if (g.currentPage == settings.size()) g.currentPage = 0;

    goToSettingsPage(g.currentPage);
}

void RecordLayer::toggleSetting(CCObject* obj) {
    CCMenuItemToggler* toggle = static_cast<CCMenuItemToggler*>(obj);
    std::string id = toggle->getID();
    auto& g = Global::get();
    mod = g.mod;

    bool value = !toggle->isToggled();

    g.mod->setSavedValue(id, value);

    // Some of these get checked every frame so idk i didnt want to do mod->getSavedValue<bool> every time
    if (id == "macro_seed_enabled") g.seedEnabled = value;
    if (id == "macro_speedhack_enabled") g.speedhackEnabled = value;
    if (id == "macro_speedhack_audio") g.speedhackAudio = value;
    if (id == "macro_show_trajectory") g.showTrajectory = value;
    if (id == "macro_coin_finder") g.coinFinder = value;
    if (id == "clickbot_enabled") g.clickbotEnabled = value;
    if (id == "p2_input_mirror") g.p2mirror = value;
    if (id == "macro_lock_delta") g.lockDelta = value;

    if (id == "macro_show_frame_label") {
        g.frameLabel = value;
        Interface::updateLabels();
    }

    if (id == "macro_frame_stepper") {
        g.frameStepper = value;
        Interface::updateButtons();
    }

    if (id == "macro_hide_recording_label" || id == "macro_hide_playing_label" || id == "render_hide_labels")
        Interface::updateLabels();

    if (id == "macro_hide_speedhack" || id == "macro_hide_stepper" || id == "macro_always_show_buttons")
        Interface::updateButtons();

    if (id == "render_only_song" && value) {
        CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
        if (RenderSettingsLayer* layer = scene->getChildByType<RenderSettingsLayer>(0)) {
            if (!layer->recordAudioToggle) return;
            layer->recordAudioToggle->toggle(false);
            g.mod->setSavedValue("render_record_audio", false);
        }
    }

    if (id == "render_record_audio" && value) {
        CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
        if (RenderSettingsLayer* layer = scene->getChildByType<RenderSettingsLayer>(0)) {
            if (!layer->onlySongToggle) return;
            layer->onlySongToggle->toggle(false);
            g.mod->setSavedValue("render_only_song", false);
        }
    }

    if (id == "menu_show_button") {
        PlayLayer* pl = PlayLayer::get();

        if (!pl) return;
        if (!pl->m_isPaused) return;

        if (PauseLayer* layer = Global::getPauseLayer()) {
            layer->onResume(nullptr);
            PlayLayer::get()->pauseGame(false);

            this->onClose(nullptr);
            RecordLayer::openMenu(true);
        }
    }
}

void RecordLayer::openKeybinds(CCObject*) {
#ifdef GEODE_IS_WINDOWS

    MoreOptionsLayer::create()->onKeybindings(nullptr);
    if (!mod->setSavedValue("opened_keybinds", true))
        FLAlertLayer::create(
            "Warning",
            "Scroll down to find xdBot's keybinds",
            "Ok"
        )->show();

#else

    Interface::openButtonEditor();

#endif
}

void RecordLayer::openPresets(CCObject*) {
    RenderPresetsLayer::create()->show();
}

void RecordLayer::onAutosaves(CCObject*) {
    std::filesystem::path path = Global::get().mod->getSaveDir() / "autosaves";

    if (std::filesystem::exists(path))
        LoadMacroLayer::open(static_cast<geode::Popup<>*>(this), nullptr, true);
    else {
        if (std::filesystem::create_directory(path))
            LoadMacroLayer::open(static_cast<geode::Popup<>*>(this), nullptr, true);
        else
            FLAlertLayer::create("Error", "There was an error getting the folder. ID: 5", "Ok")->show();
    }
}

void RecordLayer::showCodecPopup(CCObject*) {
    FLAlertLayer::create("Codec", "<cr>AMD:</c> h264_amf\n<cg>NVIDIA:</c> h264_nvenc\n<cl>INTEL:</c> h264_qsv", "Ok")->show();
}

void RecordLayer::updateDots() {
    for (int i = 0; i < dots.size(); i++) {
        if (i == Global::get().currentPage) {
            dots[i]->setScale(0.4);
            dots[i]->setOpacity(255);
        }
        else {
            dots[i]->setScale(0.3f);
            dots[i]->setOpacity(70);
        }
    }
}

bool RecordLayer::setup() {
    auto& g = Global::get();
    mod = g.mod;

    Utils::setBackgroundColor(m_bgSprite);
    
    cocos2d::CCPoint offset = (CCDirector::sharedDirector()->getWinSize() - m_mainLayer->getContentSize()) / 2;
    m_mainLayer->setPosition(m_mainLayer->getPosition() - offset);
    m_closeBtn->setPosition(m_closeBtn->getPosition() + offset);
    m_bgSprite->setPosition(m_bgSprite->getPosition() + offset);

    m_closeBtn->setPosition(m_closeBtn->getPosition() + ccp(-6.75, 6.75));
    m_closeBtn->setScale(0.675);

    menu = CCMenu::create();
    m_mainLayer->addChild(menu);

    for (int i = 0; i < settings.size(); i++) {
        CCSprite* dot = CCSprite::create("smallDot.png");
        menu->addChild(dot);
        dots.push_back(dot);
    }

    float spacing = 10.f;
    float width = (dots.size() - 1) * spacing;
    float center = 103 - (width / 2.0f);

    for (int i = 0; i < dots.size(); ++i)
        dots[i]->setPosition({ center + i * spacing, 96.5f });

    updateDots();

    warningSprite = CCSprite::createWithSpriteFrameName("geode.loader/info-alert.png");
    warningSprite->setScale(0.675f);
    warningSprite->setPosition({ 82, 307 });
    m_mainLayer->addChild(warningSprite);

    warningLabel = CCLabelBMFont::create(("WARNING: Currently recording / playing macros with a frame offset of " + std::to_string(g.frameOffset)).c_str(), "bigFont.fnt");
    warningLabel->setAnchorPoint({ 0, 0.5 });
    warningLabel->setPosition({ 92, 307 });
    warningLabel->setScale(0.275f);
    m_mainLayer->addChild(warningLabel);

    warningSprite->setVisible(g.frameOffset != 0);
    warningLabel->setVisible(g.frameOffset != 0);

    CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");

    CCLabelBMFont* versionLabel = CCLabelBMFont::create(("xdBot " + xdBotVersion).c_str(), "chatFont.fnt");
    versionLabel->setOpacity(63);
    versionLabel->setPosition(ccp(-217, -125));
    versionLabel->setAnchorPoint({ 0, 0.5 });
    versionLabel->setScale(0.4f);
    versionLabel->setSkewX(4);
    menu->addChild(versionLabel);

#ifdef GEODE_IS_WINDOWS

    CCLabelBMFont* codecBtnLbl = CCLabelBMFont::create("?", "chatFont.fnt");
    codecBtnLbl->setOpacity(148);
    codecBtnLbl->setScale(0.65f);

    CCMenuItemSpriteExtra* codecBtn = CCMenuItemSpriteExtra::create(codecBtnLbl, this, menu_selector(RecordLayer::showCodecPopup));
    codecBtn->setPosition({ -26, -49 });

    menu->addChild(codecBtn);

#endif

    CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.7f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-212, 121));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 275, 151 });
    menu->addChild(bg);

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.7f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-212, 0));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 275, 169 });
    menu->addChild(bg);

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.7f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(103, 2));
    bg->setContentSize({ 313, 339 });
    menu->addChild(bg);

    recording = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(RecordLayer::toggleRecording));
    recording->toggle(g.state == state::recording);

    playing = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(RecordLayer::togglePlaying));
    playing->toggle(g.state == state::playing);

    recording->setPosition(ccp(-161.5, 78));
    recording->setScale(0.775);
    playing->setPosition(ccp(-74.5, 78));
    playing->setScale(0.775);

    menu->addChild(recording);
    menu->addChild(playing);

    actionsLabel = CCLabelBMFont::create(("Actions: " + std::to_string(g.macro.inputs.size())).c_str(), "chatFont.fnt");
    actionsLabel->limitLabelWidth(57.f, 0.6f, 0.01f);
    actionsLabel->updateLabel();
    actionsLabel->setAnchorPoint({ 0, 0.5 });
    actionsLabel->setOpacity(83);
    actionsLabel->setPosition(ccp(-201, 110));
    menu->addChild(actionsLabel);

    CCLabelBMFont* lbl = CCLabelBMFont::create("Macro", "goldFont.fnt");
    lbl->setPosition(ccp(-116.5, 112));
    lbl->setScale(0.575f);
    menu->addChild(lbl);

    lbl = CCLabelBMFont::create("Render", "goldFont.fnt");
    lbl->setScale(0.6f);
    lbl->setPosition(ccp(-116.5, -9));
    menu->addChild(lbl);

#ifdef GEODE_IS_ANDROID
    lbl->setOpacity(90);
#endif

    lbl = CCLabelBMFont::create("Settings", "goldFont.fnt");
    lbl->setPosition(ccp(103, 111));
    lbl->setScale(0.7f);
    menu->addChild(lbl);

    lbl = CCLabelBMFont::create("Record", "bigFont.fnt");
    lbl->setPosition(ccp(-161.5, 60));
    lbl->setScale(0.325f);
    menu->addChild(lbl);

    lbl = CCLabelBMFont::create("Play", "bigFont.fnt");
    lbl->setPosition(ccp(-74, 60));
    lbl->setScale(0.325f);
    menu->addChild(lbl);

    #ifdef GEODE_IS_ANDROID
    
    lbl = CCLabelBMFont::create("Not Available For Android.", "bigFont.fnt");
    lbl->setPosition(ccp(-116, -52));
    lbl->setOpacity(120);
    lbl->setScale(0.325f);
    menu->addChild(lbl);

    #endif

    lbl = CCLabelBMFont::create("X", "chatFont.fnt");
    lbl->setPosition(ccp(-114.5, -31));
    lbl->setScale(0.7f);
    menu->addChild(lbl);

#ifdef GEODE_IS_ANDROID
    lbl->setOpacity(90);
#endif

    lbl = CCLabelBMFont::create("M", "chatFont.fnt");
    lbl->setPosition(ccp(-164, -59));
    lbl->setScale(0.7f);
    menu->addChild(lbl);

#ifdef GEODE_IS_ANDROID
    lbl->setOpacity(90);
#endif

    lbl = CCLabelBMFont::create("FPS", "chatFont.fnt");
    lbl->setPosition(ccp(-108.5, -59));
    lbl->setScale(0.49f);
    menu->addChild(lbl);

#ifdef GEODE_IS_ANDROID
    lbl->setOpacity(90);
#endif

    ButtonSprite* btnSprite = ButtonSprite::create("Save");
    btnSprite->setScale(0.54f);

    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(btnSprite,
        this,
        menu_selector(RecordLayer::openSaveMacro));

    btn->setPosition(ccp(-176, 34));
    menu->addChild(btn);

#ifdef GEODE_IS_WINDOWS
    btnSprite = ButtonSprite::create("Keybinds");
#else
    btnSprite = ButtonSprite::create("Buttons");
#endif
    btnSprite->setScale(0.54f);

    btn = CCMenuItemSpriteExtra::create(btnSprite,
        this,
        menu_selector(RecordLayer::openKeybinds));

    btn->setPosition(ccp(40, -100));
    menu->addChild(btn);


    btnSprite = ButtonSprite::create("More Settings");
    btnSprite->setScale(0.54f);

    btn = CCMenuItemSpriteExtra::create(btnSprite,
        this,
        menu_selector(RecordLayer::moreSettings));

    btn->setPosition(ccp(148, -100));
    menu->addChild(btn);

#ifdef GEODE_IS_ANDROID
    btn->setPositionX(142);
#endif

    btnSprite = ButtonSprite::create("Load");
    btnSprite->setScale(0.54f);

    btn = CCMenuItemSpriteExtra::create(btnSprite,
        this,
        menu_selector(RecordLayer::openLoadMacro));

    btn->setPosition(ccp(-115, 34));
    menu->addChild(btn);

    btnSprite = ButtonSprite::create("Edit");
    btnSprite->setScale(0.54f);

    btn = CCMenuItemSpriteExtra::create(btnSprite,
        this,
        menu_selector(RecordLayer::onEditMacro));

    btn->setPosition(ccp(-56, 34));
    menu->addChild(btn);

    widthInput = CCTextInputNode::create(150, 30, "Width", "chatFont.fnt");
    widthInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
    widthInput->ignoreAnchorPointForPosition(true);
    widthInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
    widthInput->setPosition(ccp(-157, -31));
    widthInput->setMaxLabelScale(0.7f);
    widthInput->setMouseEnabled(true);
    widthInput->setContentSize({ 60, 20 });
    widthInput->setTouchEnabled(true);
    widthInput->setMaxLabelLength(4);
    widthInput->setAllowedChars("0123456789");
    widthInput->setString(mod->getSavedValue<std::string>("render_width2").c_str());
    widthInput->setDelegate(this);
    widthInput->setID("render-input");
    menu->addChild(widthInput);

    heightInput = CCTextInputNode::create(150, 30, "Height", "chatFont.fnt");
    heightInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
    heightInput->ignoreAnchorPointForPosition(true);
    heightInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
    heightInput->setPosition(ccp(-72.5, -31));
    heightInput->setMaxLabelScale(0.7f);
    heightInput->setMouseEnabled(true);
    heightInput->setContentSize({ 60, 20 });
    heightInput->setTouchEnabled(true);
    heightInput->setMaxLabelLength(4);
    heightInput->setAllowedChars("0123456789");
    heightInput->setString(mod->getSavedValue<std::string>("render_height").c_str());
    heightInput->setDelegate(this);
    heightInput->setID("render-input");
    menu->addChild(heightInput);

    bitrateInput = CCTextInputNode::create(150, 30, "br", "chatFont.fnt");
    bitrateInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
    bitrateInput->ignoreAnchorPointForPosition(true);
    bitrateInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
    bitrateInput->setPosition(ccp(-185.5, -59));
    bitrateInput->setMaxLabelScale(0.7f);
    bitrateInput->setMouseEnabled(true);
    bitrateInput->setContentSize({ 32, 20 });
    bitrateInput->setTouchEnabled(true);
    bitrateInput->setAllowedChars("0123456789");
    bitrateInput->setMaxLabelLength(3);
    bitrateInput->setString(mod->getSavedValue<std::string>("render_bitrate").c_str());
    bitrateInput->setDelegate(this);
    menu->addChild(bitrateInput);

    CCSprite* emptyBtn = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
    emptyBtn->setScale(0.67f);
#ifdef GEODE_IS_ANDROID
    emptyBtn->setScale(-0.67f);
    emptyBtn->setOpacity(90);
#endif

    CCSprite* folderIcon = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
    folderIcon->setPosition(emptyBtn->getContentSize() / 2);
    folderIcon->setScale(0.7f);
#ifdef GEODE_IS_ANDROID
    // folderIcon->setScale(-0.7f);
    folderIcon->setOpacity(90);
#endif

    emptyBtn->addChild(folderIcon);
    btn = CCMenuItemSpriteExtra::create(
        emptyBtn,
        this,
        menu_selector(RecordLayer::openPresets)
    );
    btn->setPosition(ccp(-177.5, -97));
#ifdef GEODE_IS_ANDROID
    btn->setScale(-1);
#endif
    menu->addChild(btn);

    CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    spr->setScale(0.65f);
#ifdef GEODE_IS_ANDROID
    spr->setScale(-0.65f);
    spr->setOpacity(90);
#endif

    btn = CCMenuItemSpriteExtra::create(
        spr,
        this,
        menu_selector(RenderSettingsLayer::open)
    );
    btn->setPosition(ccp(-129.5, -97));
    menu->addChild(btn);
#ifdef GEODE_IS_ANDROID
    btn->setScale(-1);
#endif

    codecInput = CCTextInputNode::create(150, 30, "Codec", "chatFont.fnt");
    codecInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
    codecInput->ignoreAnchorPointForPosition(true);
    codecInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
    codecInput->setPosition(ccp(-70.5, -62));
    codecInput->setMouseEnabled(true);
    codecInput->setTouchEnabled(true);
    codecInput->setContentSize({ 79, 20 });
    codecInput->setScale(0.75);
    codecInput->setString(mod->getSavedValue<std::string>("render_codec").c_str());
    codecInput->setDelegate(this);
    codecInput->setAllowedChars("0123456789abcdefghijklmnopqrstuvwxyz-_.\"\\/");
    codecInput->setMaxLabelWidth(74.f);
    menu->addChild(codecInput);

    fpsInput = CCTextInputNode::create(150, 30, "FPS", "chatFont.fnt");
    fpsInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
    fpsInput->ignoreAnchorPointForPosition(true);
    fpsInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
    fpsInput->m_placeholderLabel->setScale(0.6);
    fpsInput->setPosition(ccp(-133, -59));
    fpsInput->setMaxLabelScale(0.7f);
    fpsInput->setMouseEnabled(true);
    fpsInput->setTouchEnabled(true);
    fpsInput->setContentSize({ 32, 20 });
    fpsInput->setMaxLabelLength(3);
    fpsInput->setAllowedChars("0123456789");
    fpsInput->setString(mod->getSavedValue<std::string>("render_fps").c_str());
    fpsInput->setDelegate(this);
    menu->addChild(fpsInput);

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.375f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-103.5, -21));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 162, 55 });
    bg->setZOrder(29);
    menu->addChild(bg);

#ifdef GEODE_IS_ANDROID
    bg->setOpacity(27);
#endif

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.375f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-188, -21));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 162, 55 });
    bg->setZOrder(29);
    menu->addChild(bg);

#ifdef GEODE_IS_ANDROID
    bg->setOpacity(27);
#endif

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.375f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-201.5, -49));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 82, 55 });
    bg->setZOrder(29);
    menu->addChild(bg);

#ifdef GEODE_IS_ANDROID
    bg->setOpacity(27);
#endif

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.375f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-148.5, -49));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 82, 55 });
    bg->setZOrder(29);
    menu->addChild(bg);

#ifdef GEODE_IS_ANDROID
    bg->setOpacity(27);
#endif

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setScale(0.375f);
    bg->setColor({ 0,0,0 });
    bg->setOpacity(75);
    bg->setPosition(ccp(-92, -49));
    bg->setAnchorPoint({ 0, 1 });
    bg->setContentSize({ 167, 55 });
    bg->setZOrder(29);
    menu->addChild(bg);

#ifdef GEODE_IS_ANDROID
    bg->setOpacity(27);
#endif

    ButtonSprite* spriteOn2 = ButtonSprite::create("Stop");
    spriteOn2->setScale(0.74f);
    ButtonSprite* spriteOff2 = ButtonSprite::create("Start");
    spriteOff2->setScale(0.74f);

#ifdef GEODE_IS_ANDROID
    spriteOff2->setScale(-0.74f);
#endif

    renderToggle = CCMenuItemToggler::create(spriteOff2, spriteOn2, this, menu_selector(RecordLayer::toggleRender));
    renderToggle->toggle(g.renderer.recording || g.renderer.recordingAudio);

    renderToggle->setPosition(ccp(-65.5, -100));
    menu->addChild(renderToggle);

#ifdef GEODE_IS_ANDROID
    renderToggle->setScale(-1);
    CCNode* sprite = static_cast<CCNode*>(renderToggle->m_offButton->getChildren()->objectAtIndex(0));

    if (CCLabelBMFont* lbl = typeinfo_cast<CCLabelBMFont*>(sprite->getChildren()->objectAtIndex(0)))
        lbl->setOpacity(90);

    if (CCScale9Sprite* spr = typeinfo_cast<CCScale9Sprite*>(sprite->getChildren()->objectAtIndex(1)))
        spr->setOpacity(90);

#endif

    spr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    spr->setScale(0.65f);
    btn = CCMenuItemSpriteExtra::create(
        spr,
        this,
        menu_selector(RecordLayer::macroInfo)
    );
    btn->setPosition(ccp(-36, 107));
    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    spr->setScale(0.58f);
    btn = CCMenuItemSpriteExtra::create(
        spr,
        this,
        menu_selector(RecordLayer::updatePage)
    );
    btn->setPosition(ccp(-5, 0));
    btn->setID("page-left");
    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    spr->setScale(0.58f);
    spr->setScaleX(-0.58f);
    btn = CCMenuItemSpriteExtra::create(
        spr,
        this,
        menu_selector(RecordLayer::updatePage)
    );
    btn->setPosition(ccp(209, 4.3));
    btn->setContentSize({ 26, 32.4 });
    menu->addChild(btn);
    static_cast<CCNode*>(btn->getChildren()->objectAtIndex(0))->setPositionX(13);

    for (int i = 0; i < 7; i++) {
        CCLabelBMFont* lbl = CCLabelBMFont::create("_______________________", "chatFont.fnt");
        lbl->setPosition(ccp(103, 97 - (i * 29)));
        lbl->setColor(ccc3(0, 0, 0));
        lbl->setOpacity(80);
        menu->addChild(lbl);
    }

    if (!mod->getSettingValue<bool>("restore_page"))
        g.currentPage = 0;

    goToSettingsPage(g.currentPage);

#ifdef GEODE_IS_ANDROID

    widthInput->setID("android-disabled");
    widthInput->m_placeholderLabel->setOpacity(120);

    heightInput->setID("android-disabled");
    heightInput->m_placeholderLabel->setOpacity(120);

    fpsInput->setID("android-disabled");
    fpsInput->m_placeholderLabel->setOpacity(120);

    bitrateInput->setID("android-disabled");
    bitrateInput->m_placeholderLabel->setOpacity(120);

    codecInput->setID("android-disabled");
    codecInput->m_placeholderLabel->setOpacity(120);

#endif

    CCSprite* dickordSpr = CCSprite::createWithSpriteFrameName("gj_discordIcon_001.png");
    dickordSpr->setScale(0.9f);
    CCMenuItemSpriteExtra* dickordBtn = CCMenuItemSpriteExtra::create(dickordSpr, this, menu_selector(RecordLayer::onDiscord));
    dickordBtn->setPosition((CCDirector::sharedDirector()->getWinSize() / 2 - m_size / 2 + ccp(-16, 16)));
    m_buttonMenu->addChild(dickordBtn);

    if (!Mod::get()->setSavedValue<bool>("dickord", true))
        dickordSpr->runAction(CCSequence::create(CCScaleTo::create(0.25f, 1.5f), CCRotateTo::create(0.25f, 90), CCRotateTo::create(0.25f, 180), CCRotateTo::create(0.25f, 270), CCRotateTo::create(0.25f, 0), CCScaleTo::create(0.25f, 0.9f), nullptr));

    return true;
}

void RecordLayer::setToggleMember(CCMenuItemToggler* toggle, std::string id) {
    if (id == "macro_speedhack_enabled") speedhackToggle = toggle;
    if (id == "macro_show_trajectory") trajectoryToggle = toggle;
    if (id == "macro_noclip") noclipToggle = toggle;
    if (id == "macro_frame_stepper") frameStepperToggle = toggle;
}

void RecordLayer::loadSetting(RecordSetting sett, float yPos) {
    CCLabelBMFont* lbl = CCLabelBMFont::create(sett.name.c_str(), "bigFont.fnt");
    lbl->setPosition(ccp(19.f, yPos));
    lbl->setAnchorPoint({ 0, 0.5 });
    lbl->setOpacity(200);
    lbl->setScale(sett.labelScale);

    nodes.push_back(static_cast<CCNode*>(lbl));
    menu->addChild(lbl);

    CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    float toggleScale = 0.555f;

    if (sett.disabled) {
        // Code when disabled xD!
    }

    CCMenuItemToggler* toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(RecordLayer::toggleSetting));
    toggle->setPosition(ccp(175, yPos));
    toggle->setScale(toggleScale);
    toggle->toggle(mod->getSavedValue<bool>(sett.id));
    toggle->setID(sett.id.c_str());

    nodes.push_back(static_cast<CCNode*>(toggle));
    menu->addChild(toggle);

    setToggleMember(toggle, sett.id);

    if (sett.input == InputType::None) return;

    if (sett.input == InputType::Settings) {
        CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        spr->setScale(0.419f);

        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            sett.callback
        );
        btn->setPosition(ccp(138, yPos));

        nodes.push_back(static_cast<CCNode*>(btn));
        menu->addChild(btn);
    }

    if (sett.input == InputType::Autosave) {
        CCSprite* emptyBtn = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        emptyBtn->setScale(0.469f);

        CCSprite* folderIcon = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
        folderIcon->setPosition(emptyBtn->getContentSize() / 2);
        folderIcon->setScale(0.7f);
        emptyBtn->addChild(folderIcon);

        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
            emptyBtn,
            this,
            menu_selector(RecordLayer::onAutosaves)
        );
        btn->setPosition(ccp(147, yPos));

        nodes.push_back(static_cast<CCNode*>(btn));
        menu->addChild(btn);
    }

    if (sett.input == InputType::Speedhack) {
        CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setPosition(ccp(110, yPos + 10));
        bg->setScale(0.355f);
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setAnchorPoint({ 0, 1 });
        bg->setContentSize({ 100, 55 });
        bg->setZOrder(29);
        nodes.push_back(static_cast<CCNode*>(bg));
        menu->addChild(bg);

        speedhackInput = CCTextInputNode::create(150, 30, "SH", "chatFont.fnt");
        speedhackInput->setPosition(ccp(127.5, yPos));
        speedhackInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
        speedhackInput->ignoreAnchorPointForPosition(true);
        speedhackInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
        speedhackInput->m_placeholderLabel->setScale(0.6);
        speedhackInput->setMaxLabelScale(0.7f);
        speedhackInput->setMouseEnabled(true);
        speedhackInput->setTouchEnabled(true);
        speedhackInput->setContentSize({ 32, 20 });
        speedhackInput->setAllowedChars("0123456789.");
        speedhackInput->setString(mod->getSavedValue<std::string>("macro_speedhack").c_str());
        speedhackInput->setMaxLabelWidth(30.f);
        speedhackInput->setDelegate(this);
        speedhackInput->setMaxLabelLength(4);

        nodes.push_back(static_cast<CCNode*>(speedhackInput));
        menu->addChild(speedhackInput);
    }

    if (sett.input == InputType::Seed) {
        CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setPosition(ccp(64, yPos + 10));
        bg->setScale(0.355f);
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setAnchorPoint({ 0, 1 });
        bg->setContentSize({ 258, 55 });
        bg->setZOrder(29);
        nodes.push_back(static_cast<CCNode*>(bg));
        menu->addChild(bg);

        seedInput = CCTextInputNode::create(150, 30, "Seed", "chatFont.fnt");
        seedInput->setPosition(ccp(109.5, yPos));
        seedInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
        seedInput->ignoreAnchorPointForPosition(true);
        seedInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
        seedInput->m_placeholderLabel->setScale(0.6);
        seedInput->setMaxLabelScale(0.7f);
        seedInput->setMouseEnabled(true);
        seedInput->setTouchEnabled(true);
        seedInput->setContentSize({ 85, 20 });
        seedInput->setAllowedChars("0123456789");
        seedInput->setString(mod->getSavedValue<std::string>("macro_seed").c_str());
        seedInput->setMaxLabelWidth(70.f);
        seedInput->setDelegate(this);
        seedInput->setMaxLabelLength(20);

        nodes.push_back(static_cast<CCNode*>(seedInput));
        menu->addChild(seedInput);
    }

    if (sett.input == InputType::Respawn) {
        CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setPosition(ccp(110, yPos + 10));
        bg->setScale(0.355f);
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setAnchorPoint({ 0, 1 });
        bg->setContentSize({ 100, 55 });
        bg->setZOrder(29);
        nodes.push_back(static_cast<CCNode*>(bg));
        menu->addChild(bg);

        respawnInput = CCTextInputNode::create(150, 30, "sec", "chatFont.fnt");
        respawnInput->setPosition(ccp(127.5, yPos));
        respawnInput->m_textField->setAnchorPoint({ 0.5f, 0.5f });
        respawnInput->ignoreAnchorPointForPosition(true);
        respawnInput->m_placeholderLabel->setAnchorPoint({ 0.5f, 0.5f });
        respawnInput->m_placeholderLabel->setScale(0.6);
        respawnInput->setMaxLabelScale(0.7f);
        respawnInput->setMouseEnabled(true);
        respawnInput->setTouchEnabled(true);
        respawnInput->setContentSize({ 32.f, 20.f });
        respawnInput->setAllowedChars("0123456789.");
        respawnInput->setString(fmt::format("{:.2}", mod->getSavedValue<double>("respawn_time")).c_str());
        respawnInput->setMaxLabelWidth(30.f);
        respawnInput->setDelegate(this);
        respawnInput->setMaxLabelLength(4);

        nodes.push_back(static_cast<CCNode*>(respawnInput));
        menu->addChild(respawnInput);
    }
}

void RecordLayer::goToSettingsPage(int page) {
    checkSpeedhack();

    for (size_t i = 0; i < nodes.size(); i++)
        nodes[i]->removeFromParentAndCleanup(false);

    nodes.clear();

    speedhackToggle = nullptr;
    frameStepperToggle = nullptr;
    trajectoryToggle = nullptr;
    noclipToggle = nullptr;
    speedhackInput = nullptr;
    respawnInput = nullptr;
    seedInput = nullptr;

    for (size_t i = 0; i < 6; i++)
        loadSetting(settings[page][i], ySettingPositions[i]);

    updateDots();
}

void RecordLayer::onDiscord(CCObject*) {
    geode::createQuickPopup(
        "Discord",
        "Join the <cb>Discord</c> server?\n(<cl>discord.gg/rWdZbw7aKm</c>).\n<cr>If you are in an unofficial server, you should leave NOW.</c>",
        "No", "Yes",
        [](auto, bool btn2) {
        	if (btn2)
				geode::utils::web::openLinkInBrowser("https://discord.gg/rWdZbw7aKm");
        }
    );
}