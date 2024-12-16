#pragma once

#include "../includes.hpp"

class RenderSettingsLayer : public geode::Popup<>, public TextInputDelegate {
	
public:

	Slider* sfxSlider = nullptr;
	Slider* musicSlider = nullptr;
	TextInput* fadeInInput = nullptr;
	TextInput* fadeOutInput = nullptr;

	CCTextInputNode* argsInput = nullptr;
	CCTextInputNode* audioArgsInput = nullptr;
	CCTextInputNode* secondsInput = nullptr;
	CCTextInputNode* videoArgsInput = nullptr;

	CCMenuItemToggler* onlySongToggle = nullptr;
	CCMenuItemToggler* recordAudioToggle = nullptr;

	Mod* mod = nullptr;

private:

	bool setup() override;

public:

	STATIC_CREATE(RenderSettingsLayer, 396, 277)
	
	void open(CCObject*) {
		create()->show();
	}

	void close(CCObject*) {
		keyBackClicked();
	}

	void textChanged(CCTextInputNode* node) override;

	void onSlider(CCObject*);

	void onDefaults(CCObject*);
};