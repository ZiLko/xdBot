#include "../includes.hpp"
#include "record_layer.hpp"

class MirrorSettingsLayer : public geode::Popup<> {

private:
	
    bool setup() override {
        setTitle("Input Mirror");
		
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

        CCLabelBMFont* lbl = CCLabelBMFont::create("Inverted", "bigFont.fnt");
		lbl->setPosition({17, -0});
		lbl->setScale(0.5f);
		menu->addChild(lbl);

        CCMenuItemToggler* toggle = CCMenuItemToggler::create(spriteOff, spriteOn, this, menu_selector(RecordLayer::toggleSetting));
		toggle->setPosition({-47, -0});
		toggle->setScale(0.875f);
		toggle->toggle(Mod::get()->getSavedValue<bool>("p2_input_mirror_inverted"));
		toggle->setID("p2_input_mirror_inverted");
        menu->addChild(toggle);

		return true;
    }

public:

	STATIC_CREATE(MirrorSettingsLayer, 200, 146)
	
	void open(CCObject*) {
		create()->show();
	}

};