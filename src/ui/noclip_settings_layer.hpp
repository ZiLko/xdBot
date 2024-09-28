#include "../includes.hpp"

class NoclipSettingsLayer : public geode::Popup<> {

private:
	
    bool setup() override {
    	Utils::setBackgroundColor(m_bgSprite);

        setTitle("Noclip");

        CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");

        CCLabelBMFont* lbl = CCLabelBMFont::create("Player 1", "bigFont.fnt");
		lbl->setPosition({17, 21});
		lbl->setScale(0.5f);
		m_buttonMenu->addChild(lbl);

        CCMenuItemToggler* toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(NoclipSettingsLayer::onToggle));
		toggle->setPosition({-47, 21});
		toggle->setScale(0.875f);
		toggle->setID("p1");
		toggle->toggle(Mod::get()->getSavedValue<bool>("macro_noclip_p1"));
        m_buttonMenu->addChild(toggle);

		lbl = CCLabelBMFont::create("Player 2", "bigFont.fnt");
		lbl->setPosition({17, -35});
		lbl->setScale(0.5f);
		m_buttonMenu->addChild(lbl);

		toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(NoclipSettingsLayer::onToggle));
		toggle->setPosition({-47, -35});
		toggle->setScale(0.875f);
		toggle->setID("p2");
		toggle->toggle(Mod::get()->getSavedValue<bool>("macro_noclip_p2"));
        m_buttonMenu->addChild(toggle);

		return true;
    }

public:

	STATIC_CREATE(NoclipSettingsLayer, 200, 176)
	
	void open(CCObject*) {
		create()->show();
	}

	void onToggle(CCObject* obj) {
		CCMenuItemToggler* toggle = typeinfo_cast<CCMenuItemToggler*>(obj);
		if (!toggle) return;

		std::string id = toggle->getID();
		
		Mod::get()->setSavedValue("macro_noclip_" + id, !toggle->isToggled());
	}

};