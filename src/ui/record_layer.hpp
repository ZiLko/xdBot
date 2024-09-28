#pragma once

#include <Geode/ui/GeodeUI.hpp>
#include "../includes.hpp"

#include "load_macro_layer.hpp"
#include "render_settings_layer.hpp"
#include "save_macro_layer.hpp"
#include "macro_info_layer.hpp"
#include "clickbot_layer.hpp"

enum InputType {
	None,
	Settings,
	Autosave,
	Speedhack,
	Seed
};

struct RecordSetting {
	std::string name;
	std::string id;
	InputType input;
	float labelScale = 0.325f;
	bool disabled = false;
};

const float ySettingPositions[6]{
	76.5f, 47.5f, 18.5f, -11.5f, -40.5f, -69.5f
};

const std::vector<std::vector<RecordSetting>> settings{
	{
		{ "Speedhack:", "macro_speedhack_enabled", InputType::Speedhack, 0.4f },
		{ "Seed:", "macro_seed_enabled", InputType::Seed, 0.4f },
		{ "Show Trajectory:", "macro_show_trajectory", InputType::None },
		{ "Enable Coin Finder:", "macro_coin_finder", InputType::None },
		{ "Enable Noclip:", "macro_noclip", InputType::Settings },
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
		{ "Enable Clickbot:", "clickbot_enabled", InputType::Settings},
		{ "Speedhack Audio:", "macro_speedhack_audio", InputType::None },
		{ "Always Practice Fixes:", "macro_always_practice_fixes", InputType::None },
		{ "Show Frame Label:", "macro_show_frame_label", InputType::None },
		{ "Ignore inputs:", "macro_ignore_inputs", InputType::None },
		{ "Auto Stop Playing:", "macro_auto_stop_playing", InputType::None }
	}
};

class RecordLayer : public geode::Popup<>, public TextInputDelegate {
public:
	CCMenuItemToggler* recording = nullptr;
	CCMenuItemToggler* playing = nullptr;
	CCMenuItemToggler* speedhackToggle = nullptr;
	CCMenuItemToggler* trajectoryToggle = nullptr;
	CCMenuItemToggler* noclipToggle = nullptr;
	CCMenuItemToggler* frameStepperToggle = nullptr;
	CCMenuItemToggler* renderToggle = nullptr;

	CCLabelBMFont* actionsLabel = nullptr;
	CCLabelBMFont* fpsLabel = nullptr;
	CCLabelBMFont* warningLabel = nullptr;

	CCSprite* warningSprite = nullptr;

	CCMenuItemSpriteExtra* FPSLeft = nullptr;
	CCMenuItemSpriteExtra* FPSRight = nullptr;

	CCTextInputNode* widthInput = nullptr;
	CCTextInputNode* heightInput = nullptr;
	CCTextInputNode* bitrateInput = nullptr;
	CCTextInputNode* fpsInput = nullptr;
	CCTextInputNode* codecInput = nullptr;
	CCTextInputNode* seedInput = nullptr;
	CCTextInputNode* speedhackInput = nullptr;

	std::vector<CCNode*> nodes;
	std::vector<CCSprite*> dots;

	CCMenu* menu = nullptr;

	Mod* mod = nullptr;

	bool cursorWasHidden = false;

protected:

	bool setup() override;

	~RecordLayer() override {
		cocos2d::CCTouchDispatcher::get()->unregisterForcePrio(this);
	    Global::get().layer = nullptr;
	}

public:

	STATIC_CREATE(RecordLayer, 455, 271)
	
	virtual void onClose(cocos2d::CCObject*) override;

	void textChanged(CCTextInputNode* node) override;

	void checkSpeedhack();

	static RecordLayer* openMenu(bool instant = false);

	void openMenu2(CCObject*) {
		RecordLayer::openMenu();
	}

	void moreSettings(CCObject*) {
		geode::openSettingsPopup(mod);
	}

	void updateDots();

	void openLoadMacro(CCObject*);

	void openSaveMacro(CCObject*);

	void showCodecPopup(CCObject*);

	void toggleRecording(CCObject*);

	void togglePlaying(CCObject*);

	void toggleRender(CCObject* btn);

	void openRendersFolder(CCObject*);

	void onAutosaves(CCObject*);

	void goToSettingsPage(int page);

	void loadSetting(RecordSetting sett, float yPos);

	void setToggleMember(CCMenuItemToggler* toggle, std::string id);

	void onEditMacro(CCObject*);

	void macroInfo(CCObject*);

	void updatePage(CCObject* obj);

	void toggleSetting(CCObject* obj);

	void openKeybinds(CCObject*);

	void toggleFPS(bool on);

};
