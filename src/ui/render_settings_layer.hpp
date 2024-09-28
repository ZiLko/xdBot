#pragma once

#include "../includes.hpp"

class RenderSettingsLayer : public geode::Popup<>, public TextInputDelegate {
	CCTextInputNode* argsInput = nullptr;
	CCTextInputNode* audioArgsInput = nullptr;
	CCTextInputNode* secondsInput = nullptr;
	CCTextInputNode* videoArgsInput = nullptr;

	Mod* mod = nullptr;

private:

	bool setup() override;

public:

	STATIC_CREATE(RenderSettingsLayer, 268, 277)
	
	void open(CCObject*) {
		create()->show();
	}

	void close(CCObject*) {
		keyBackClicked();
	}

	void textChanged(CCTextInputNode* node) override;

	void onDefaults(CCObject*);
};