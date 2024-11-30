#include "../includes.hpp"

class NoclipSettingsLayer : public geode::Popup<> {

private:
	
    bool setup() override {
        setTitle("Noclip");
		
		cocos2d::CCPoint offset = (CCDirector::sharedDirector()->getWinSize() - m_mainLayer->getContentSize()) / 2;
		m_mainLayer->setPosition(m_mainLayer->getPosition() - offset);
		m_closeBtn->setPosition(m_closeBtn->getPosition() + offset);
		m_bgSprite->setPosition(m_bgSprite->getPosition() + offset);
        m_title->setPosition(m_title->getPosition() + offset);

    	Utils::setBackgroundColor(m_bgSprite);
		CCMenu* menu = CCMenu::create();
		m_mainLayer->addChild(menu);

        CCSprite* spriteOn = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
        CCSprite* spriteOff = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");

        CCLabelBMFont* lbl = CCLabelBMFont::create("Player 1", "bigFont.fnt");
		lbl->setPosition({17, 21});
		lbl->setScale(0.5f);
		menu->addChild(lbl);

        CCMenuItemToggler* toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(NoclipSettingsLayer::onToggle));
		toggle->setPosition({-47, 21});
		toggle->setScale(0.875f);
		toggle->setID("p1");
		toggle->toggle(Mod::get()->getSavedValue<bool>("macro_noclip_p1"));
        menu->addChild(toggle);

		lbl = CCLabelBMFont::create("Player 2", "bigFont.fnt");
		lbl->setPosition({17, -35});
		lbl->setScale(0.5f);
		menu->addChild(lbl);

		toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(NoclipSettingsLayer::onToggle));
		toggle->setPosition({-47, -35});
		toggle->setScale(0.875f);
		toggle->setID("p2");
		toggle->toggle(Mod::get()->getSavedValue<bool>("macro_noclip_p2"));
        menu->addChild(toggle);

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