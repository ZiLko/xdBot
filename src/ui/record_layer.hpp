#pragma once

#include <Geode/ui/GeodeUI.hpp>
#include "../includes.hpp"

#include "load_macro_layer.hpp"
#include "render_settings_layer.hpp"
#include "save_macro_layer.hpp"
#include "macro_info_layer.hpp"

enum InputType {
	None,
	Settings,
	Autosave,
	Speedhack,
	Seed,
	Respawn
};

struct RecordSetting {
	std::string name;
	std::string id;
	InputType input;
	float labelScale = 0.325f;
	cocos2d::SEL_MenuHandler callback = nullptr;
	bool disabled = false;
};

const float ySettingPositions[6]{
	76.5f, 47.5f, 18.5f, -11.5f, -40.5f, -69.5f
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
	CCTextInputNode* respawnInput = nullptr;

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
		geode::openSettingsPopup(mod, false);
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