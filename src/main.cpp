#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <cocos2d.h>
#include <vector>
#include <chrono>
#include "fileSystem.hpp"

Mod* mod = nullptr;

float leftOver = 0.f; // For CCScheduler

double prevSpeed = 1.0f;

int fixedFps = 240;
int androidFps = 60;
int fpsIndex = 0;

#ifdef GEODE_IS_ANDROID
	int offset = 0x320;
#else
	int offset = 0x328;
#endif

bool safeModeEnabled = false;
bool restart = false;
bool stepFrame = false;
bool playerHolding = false;
bool lastHold = false;
bool playingAction = false;
bool shouldPlay = false;
bool shouldPlay2 = false;
bool holdV = false;
bool playedMacro = false;
bool noDelayedReset = false;

const int playerEnums[2][3] = {
    {cocos2d::enumKeyCodes::KEY_ArrowUp, cocos2d::enumKeyCodes::KEY_ArrowLeft, cocos2d::enumKeyCodes::KEY_ArrowRight}, 
    {cocos2d::enumKeyCodes::KEY_W, cocos2d::enumKeyCodes::KEY_A, cocos2d::enumKeyCodes::KEY_D}
};

const int fpsArr[4] = {60,120,180,240};

void releaseKeys() {
	for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
			cocos2d::CCKeyboardDispatcher::get()->dispatchKeyboardMSG(static_cast<cocos2d::enumKeyCodes>(playerEnums[row][col]), false, false);
        }
    }
}

bool areEqual(float a, float b) {
    return std::abs(a - b) < 0.1f;
}

CCLabelBMFont* frameLabel = nullptr;
CCLabelBMFont* stateLabel = nullptr;

CCMenu* buttonsMenu = nullptr;
CCMenuItemSpriteExtra* advanceFrameBtn = nullptr;
CCMenuItemSpriteExtra* disableFSBtn = nullptr;
CCMenuItemSpriteExtra* speedhackBtn = nullptr;

using namespace geode::prelude;

namespace safeMode {
using opcode = std::pair<unsigned long, std::vector<uint8_t>>;

	inline const std::array<opcode, 15> codes{
		opcode{ 0x2DDC7E, { 0x0F, 0x84, 0xCA, 0x00, 0x00, 0x00 } },
		{ 0x2DDD6A, { 0x0F, 0x84, 0xEA, 0x01, 0x00, 0x00 } },
		{ 0x2DDD70, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
		{ 0x2DDD77, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
		{ 0x2DDEE5, { 0x90 } },
		{ 0x2DDF6E, { 0x0F, 0x84, 0xC2, 0x02, 0x00, 0x00 } },

		{ 0x2E6BDE, { 0x90, 0xE9, 0xAD, 0x00, 0x00, 0x00 } },
		{ 0x2E6B32, { 0xEB, 0x0D } },
		{ 0x2E69F4, { 0x0F, 0x4C, 0xC1 } },
		{ 0x2E6993, { 0x90, 0xE9, 0x85, 0x01, 0x00, 0x00 } },

		{ 0x2EACD0, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
		{ 0x2EACD6, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
		{ 0x2EACF7, { 0x90 } },
		
		{ 0x2EA81F, { 0x6A, 0x00 } },
		{ 0x2EA83D, { 0x90 } }
	};
	inline std::array<geode::Patch*, 15> patches;

	void updateSafeMode() {
		for (auto& patch : patches) {
		if (safeModeEnabled && !isAndroid) {
			if (!patch->isEnabled()) {
				try {
					patch->enable();
				} catch(const std::exception& e) {
					log::debug("wtf? - {}", e);
				}
			}
		} else {
			if (patch->isEnabled()) {
				try {
					patch->disable();
				} catch(const std::exception& e) {
					log::debug("wtf 2? - {}", e);
				}
			}
		}
	}
	}
}

struct playerData {
	float xPos;
	float yPos;
	bool upsideDown;
	float rotation;
	double xSpeed;
	double ySpeed;
};

struct data {
    bool player1;
    int frame;
    int button;
    bool holding;
	bool posOnly;
	playerData p1;
	playerData p2;
};

data* androidAction = nullptr;

enum state {
    off,
    recording,
    playing
};

class recordSystem {
public:
	bool android = false;
	int fps = 0;
    state state = off;
 	size_t currentAction = 0;
   	std::vector<data> macro;

	int currentFrame() {
		int fps2 = (android) ? 240 : fps;
		return static_cast<int>((*(double*)(((char*)PlayLayer::get()) + offset)) * fps2);
	}
	void syncMusic() {
		auto playLayer = PlayLayer::get();
		if (playLayer->m_levelSettings->m_platformerMode) return;
		FMODAudioEngine::sharedEngine()->setMusicTimeMS(
			(currentFrame()*1000)/240 + playLayer->m_levelSettings->m_songOffset*1000,
			true,
			0
		);
	}
	void recordAction(bool holding, int button, bool player1, int frame, GJBaseGameLayer* bgl, playerData p1Data, playerData p2Data) {
		bool realp1;
		if (isAndroid) {
			bool plat = bgl->m_levelSettings->m_platformerMode;
			realp1 = (GameManager::get()->getGameVariable("0010") && (!plat || button == 1)) ? !player1 : player1;
			if (macro.size() >= 2 && plat) {
					if (macro.back().holding == macro[macro.size()-2].holding &&
					 macro.back().frame == macro[macro.size()-2].frame && macro[macro.size()-2].button != 1)
						macro.pop_back();
			}
		}
		else realp1 = player1;
		
    	macro.push_back({realp1, frame, button, holding, false, p1Data, p2Data});
	}

};

recordSystem recorder;

class RecordLayer : public geode::Popup<std::string const&> {
	CCLabelBMFont* fpsLabel = nullptr;
 	CCLabelBMFont* infoMacro = nullptr;
 	CCMenuItemToggler* recording = nullptr;
    CCMenuItemToggler* playing = nullptr;
protected:
    bool setup(std::string const& value) override {
        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
		auto versionLabel = CCLabelBMFont::create("xdBot v1.5.3 - made by Zilko", "chatFont.fnt");
		versionLabel->setOpacity(60);
		versionLabel->setAnchorPoint(ccp(0.0f,0.5f));
		versionLabel->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(3, 6));
		versionLabel->setScale(0.5f);
		this->addChild(versionLabel);
		this->setTitle("xdBot");
		auto menu = CCMenu::create();
    	menu->setPosition({0, 0});
    	m_mainLayer->addChild(menu);

 		auto checkOffSprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
   		auto checkOnSprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

		CCPoint topLeftCorner = winSize/2.f-ccp(m_size.width/2.f,-m_size.height/2.f);

		auto label = CCLabelBMFont::create("Record", "bigFont.fnt"); 
    	label->setAnchorPoint({0, 0.5});
    	label->setScale(0.7f);
    	label->setPosition(topLeftCorner + ccp(168, -60));
    	m_mainLayer->addChild(label);

    	recording = CCMenuItemToggler::create(checkOffSprite,
		checkOnSprite,
		this,
		menu_selector(RecordLayer::toggleRecord));

    	recording->setPosition(label->getPosition() + ccp(105,0));
    	recording->setScale(0.85f);
    	recording->toggle(recorder.state == state::recording); 
    	menu->addChild(recording);

    	auto spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    	spr->setScale(0.8f);
    	auto btn = CCMenuItemSpriteExtra::create(
        	spr,
        	this,
        	menu_selector(RecordLayer::openSettingsMenu)
    	);
    	btn->setPosition(winSize/2.f-ccp(m_size.width/2.f,m_size.height/2.f) + ccp(325, 20));
    	menu->addChild(btn);

		spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    	spr->setScale(0.8f);
    	btn = CCMenuItemSpriteExtra::create(
        	spr,
        	this,
        	menu_selector(RecordLayer::updateFps)
    	);
    	btn->setPosition(winSize/2.f-ccp(-m_size.width/2.f,m_size.height/2.f) + ccp(-78, 80));
		btn->setID("left");
    	menu->addChild(btn);

		spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    	spr->setScale(0.8f);
    	btn = CCMenuItemSpriteExtra::create(
        	spr,
        	this,
        	menu_selector(RecordLayer::updateFps)
    	);
    	btn->setPosition(winSize/2.f-ccp(-m_size.width/2.f,m_size.height/2.f) + ccp(-16, 80));
		btn->setID("right");
    	menu->addChild(btn);

		if (!isAndroid) {
		spr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    	spr->setScale(0.65f);
    	btn = CCMenuItemSpriteExtra::create(
        	spr,
        	this,
        	menu_selector(RecordLayer::keyInfo)
    	);
    	btn->setPosition(topLeftCorner + ccp(290, -10));
    	menu->addChild(btn);
		}

		label = CCLabelBMFont::create("FPS", "bigFont.fnt");
    	label->setScale(0.6f);
    	label->setPosition(winSize/2.f-ccp(-m_size.width/2.f,m_size.height/2.f) + ccp(-129, 80)); 
    	label->setAnchorPoint({0, 0.5});
    	m_mainLayer->addChild(label);

		fpsLabel = CCLabelBMFont::create(std::to_string(fpsArr[fpsIndex]).c_str(), "bigFont.fnt");
    	fpsLabel->setScale(0.6f);
    	fpsLabel->setPosition(winSize/2.f-ccp(-m_size.width/2.f,m_size.height/2.f) + ccp(-47, 80)); 
    	fpsLabel->setAnchorPoint({0.5, 0.5});
    	m_mainLayer->addChild(fpsLabel);

    	label = CCLabelBMFont::create("Play", "bigFont.fnt");
    	label->setScale(0.7f);
    	label->setPosition(topLeftCorner + ccp(198, -90)); 
    	label->setAnchorPoint({0, 0.5});
    	m_mainLayer->addChild(label);



     	playing = CCMenuItemToggler::create(checkOffSprite, checkOnSprite,
	 	this,
	 	menu_selector(RecordLayer::togglePlay));

    	playing->setPosition(label->getPosition() + ccp(75,0)); 
    	playing->setScale(0.85f);
    	playing->toggle(recorder.state == state::playing); 
    	menu->addChild(playing);

 		auto btnSprite = ButtonSprite::create("Save");
    	btnSprite->setScale(0.72f);

   		btn = CCMenuItemSpriteExtra::create(btnSprite,
   		this,
   		menu_selector(saveMacroPopup::openSaveMacro));

    	btn->setPosition(topLeftCorner + ccp(65, -160)); 
    	menu->addChild(btn);

		btnSprite = ButtonSprite::create("Load");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(loadMacroPopup::openLoadMenu));

    	btn->setPosition(topLeftCorner + ccp(144, -160));
    	menu->addChild(btn);

  		btnSprite = ButtonSprite::create("Clear");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(RecordLayer::clearMacro));

    	btn->setPosition(topLeftCorner + ccp(228, -160));
    	menu->addChild(btn);

		infoMacro = CCLabelBMFont::create("", "chatFont.fnt");
    	infoMacro->setAnchorPoint({0, 1});
    	infoMacro->setPosition(topLeftCorner + ccp(34, -44));
		updateInfo();
    	m_mainLayer->addChild(infoMacro);

        return true;
	}

    static RecordLayer* create() {
        auto ret = new RecordLayer();
        if (ret && ret->init(300, 200, "", "GJ_square02.png")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

public:
 
	void openSettingsMenu(CCObject*) {
		geode::openSettingsPopup(mod);
	}

	void updateFps(CCObject* ob) {
		if (static_cast<CCLabelBMFont*>(ob)->getID() == "left")
			fpsIndex--;
		else if (static_cast<CCLabelBMFont*>(ob)->getID() == "right") {
			fpsIndex++;
		}

		if (fpsIndex == -1) fpsIndex = 3;
		else if (fpsIndex == 4) fpsIndex = 0;

		fpsLabel->setString(std::to_string(fpsArr[fpsIndex]).c_str());
		mod->setSavedValue<float>("previous_fps", fpsIndex);
	}


	void keyInfo(CCObject*) {
		FLAlertLayer::create(
    		"Shortcuts",   
    		"<cg>Toggle Speedhack</c> = <cl>C</c>\n<cg>Advance Frame</c> = <cl>V</c>\n<cg>Disable Frame Stepper</c> = <cl>B</c>",  
    		"OK"      
		)->show();
	}

	void updateInfo() {
		int clicksCount = 0;
		if (!recorder.macro.empty()) {
			for (const data& element : recorder.macro) {
        		if (element.holding && !element.posOnly) clicksCount++;
    		}
		}
		
 		std::stringstream infoText;

		infoText << "Current Macro:";
		
		infoText << "\nClicks: " << clicksCount;

		infoText << "\nDuration: " << (!recorder.macro.empty() 
		? recorder.macro.back().frame / fixedFps : 0) << "s";

		infoText << "\nFPS: " << (recorder.fps);

    	infoMacro->setString(infoText.str().c_str());
	}

	void togglePlay(CCObject*) {
		if (recorder.state == state::recording) recording->toggle(false);
    	recorder.state = (recorder.state == state::playing) ? state::off : state::playing;

		if (recorder.state == state::playing) {
			restart = true;
			mod->setSettingValue("speedhack", 1.0);
			if (mod->getSettingValue<bool>("speedhack_audio")) {
				FMOD::ChannelGroup* channel;
    			FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
				channel->setPitch(1);
			}
		}
		else if (recorder.state == state::off) restart = false;
		mod->setSettingValue("frame_stepper", false);
	}

	void toggleRecord(CCObject* sender) {
			if (recorder.state == state::playing) this->playing->toggle(false);
    		recorder.state = (recorder.state == state::recording) 
			? state::off : state::recording;
			if (recorder.state == state::recording) {
				if (recorder.macro.empty()) {
					recorder.android = false;
					recorder.fps = fpsArr[fpsIndex];
				}
					
				restart = true;
				updateInfo();
			} else if (recorder.state == state::off) {
				restart = false;
				mod->setSettingValue("frame_stepper", false);
			}
	}

	void clearMacro(CCObject*) {
		if (recorder.macro.empty()) return;
		geode::createQuickPopup(
    	"Clear Macro",     
    	"<cr>Clear</c> the current macro?", 
    	"Cancel", "Yes",  
    	[this](auto, bool btn2) {
        	if (btn2) {
				recorder.macro.clear();
				this->updateInfo();
				if (recorder.state == state::playing) this->playing->toggle(false);
				if (recorder.state == state::recording) this->recording->toggle(false);
				recorder.state = state::off;
			}
    	});
	}

    void openMenu(CCObject*) {
		auto layer = create();
		layer->m_noElasticity = (static_cast<float>(mod->getSettingValue<double>("speedhack")) < 1
		 && recorder.state == state::recording) ? true : false;
		layer->show();
	}
};

void saveMacroPopup::openSaveMacro(CCObject*) {
	if (recorder.macro.empty()) {
		FLAlertLayer::create(
    	"Save Macro",   
    	"You can't save an <cl>empty</c> macro.",  
    	"OK"      
		)->show();
		return;
	}
	auto layer = create();
	layer->m_noElasticity = (static_cast<float>(mod->getSettingValue<double>("speedhack")) < 1
	 && recorder.state == state::recording) ? true : false;
	layer->show();
}

void saveMacroPopup::saveMacro(CCObject*) {
  if (std::string(macroNameInput->getString()).length() < 1) {
		FLAlertLayer::create(
    	"Save Macro",   
    	"Macro name can't be <cl>empty</c>.",  
    	"OK"      
		)->show();
		return;
	}

	std::string savePath = mod->getSaveDir().string()
     +slash+std::string(macroNameInput->getString()) + ".xd";

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(savePath);
	std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);

	std::wifstream tempFile(wideString);

    if (tempFile.good()) {
        FLAlertLayer::create(
    	"Save Macro",   
    	"A macro with this <cl>name</c> already exists.",  
    	"OK"      
		)->show();
		tempFile.close();
		return;
    }
	tempFile.close();

    std::wofstream file(wideString);
    file.imbue(utf8_locale);

	if (file.is_open()) {

		file << recorder.fps << "\n";

		for (auto &action : recorder.macro) {
			file << action.frame << "|" << action.holding <<
			"|" << action.button << "|" << action.player1 <<
			"|" << action.posOnly << "|" << action.p1.xPos <<
			"|" << action.p1.yPos << "|" << action.p1.upsideDown <<
			"|" << action.p1.rotation << "|" << action.p1.xSpeed <<
			"|" << action.p1.ySpeed << "|" << action.p2.xPos <<
			"|" << action.p2.yPos << "|" << action.p2.upsideDown <<
			"|" << action.p2.rotation << "|" << action.p2.xSpeed <<
			"|" << action.p2.ySpeed  << "\n";
		}
		file.close();
		CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
		CCObject* child;
		CCARRAY_FOREACH(children, child) {
    		saveMacroPopup* saveLayer = dynamic_cast<saveMacroPopup*>(child);
    		if (saveLayer) {
        		saveLayer->keyBackClicked();
				break;
   			}
		}
        FLAlertLayer::create(
    	"Save Macro",   
    	"Macro saved <cg>successfully</c>.",  
    	"OK"      
		)->show();
	} else {
        FLAlertLayer::create(
    	"Save Macro",   
    	"There was an <cr>error</c> saving the macro.",  
    	"OK"      
		)->show();
    }
}

void macroCell::handleLoad(CCObject* btn) {
	std::string loadPath = mod->getSaveDir().string()
    +slash+static_cast<CCMenuItemSpriteExtra*>(btn)->getID() + ".xd";
	recorder.macro.clear();

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(loadPath);
	std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);


    std::wifstream file(wideString);
    file.imbue(utf8_locale);
	std::wstring line;
	if (!file.is_open()) {
		FLAlertLayer::create(
    	"Load Macro",   
    	"An <cr>error</c> occurred while loading this macro.",  
    	"OK"      
		)->show();
		return;
	}
	bool firstIt = true;
	bool andr = false;
	while (std::getline(file, line)) {
		std::wistringstream isSS(line);

		playerData p1;
		playerData p2;

		int holding, frame, button, player1, posOnly;
		float p1xPos, p1yPos, p1rotation, p1xSpeed, p1ySpeed;
		float p2xPos, p2yPos, p2rotation, p2xSpeed, p2ySpeed;
		int p1upsideDown, p2upsideDown;

		wchar_t s;
		int count = 0;
    	for (char ch : line) {
        	if (ch == '|') {
            	count++;
        	}
    	}

		if (count > 3) {
			if (isSS >> frame >> s >> holding >> s >> button >> 
			s >> player1 >> s >> posOnly >> s >>
			p1xPos >> s >> p1yPos >> s >> p1upsideDown
		 	>> s >> p1rotation >> s >> p1xSpeed >> s >>
		 	p1ySpeed >> s >> p2xPos >> s >> p2yPos >> s >> p2upsideDown
		 	>> s >> p2rotation >> s >> p2xSpeed >> s >>
		 	p2ySpeed && s == L'|') {
				p1 = {
					(float)p1xPos,
					(float)p1yPos,
					(bool)p1upsideDown,
					(float)p1rotation,
					(double)p1xSpeed,
					(double)p1ySpeed,
				};
				p2 = {
					(float)p2xPos,
					(float)p2yPos,
					(bool)p2upsideDown,
					(float)p2rotation,
					(double)p2xSpeed,
					(double)p2ySpeed,
				};
				if ((isAndroid && !(bool)posOnly) || !isAndroid)
					recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding, (bool)posOnly, p1, p2});
			}
		} else if (count < 1) {
			std::wstring andStr;
			int fps;
			if (firstIt) {
    			if (isSS >> fps) {
					andr = true;
					recorder.android = false;
					recorder.fps = (int)fps;
    			} else {
        			isSS.clear();
        			if (isSS >> andStr && andStr == L"android") {
            			andr = true;
            			recorder.android = true;
						recorder.fps = 60;
        			}
    			}
			}
		} else {
			if (isSS >> frame >> s >> holding >> s >> button >> 
			s >> player1 && s == L'|') {
				p1.xPos = 0;
				recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding, false, p1, p2});
			}
		}
    	firstIt = false;
	}
	if (!andr) {
		recorder.android = false;
		recorder.fps = 240;
	}

	CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
	CCObject* child;
	CCARRAY_FOREACH(children, child) {
    	RecordLayer* recordLayer = dynamic_cast<RecordLayer*>(child);
    	loadMacroPopup* loadLayer = dynamic_cast<loadMacroPopup*>(child);
    	if (recordLayer) {
        	recordLayer->updateInfo();
    	} else if (loadLayer) loadLayer->keyBackClicked();
	}
	file.close();
	FLAlertLayer::create(
    "Load Macro",   
    "Macro loaded <cg>successfully</c>.",  
    "OK"      
	)->show();
}

void macroCell::loadMacro(CCObject* button) {
	if (!recorder.macro.empty()) {
		geode::createQuickPopup(
    	"Load Macro",     
    	"<cr>Overwrite</c> the current macro?", 
    	"Cancel", "Ok",  
    	[this, button](auto, bool btn2) {
        	if (btn2) this->handleLoad(button);
    	}); 
	} else handleLoad(button);
}

void clearState(bool safeMode) {
	if (mod->getSettingValue<bool>("speedhack_audio")) {
		FMOD::ChannelGroup* channel;
    	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
		channel->setPitch(1);
	}
	
	recorder.state = state::off;

	playingAction = false;

	if (isAndroid && PlayLayer::get()) {
		if (disableFSBtn != nullptr) {
			disableFSBtn->removeFromParent();
			disableFSBtn = nullptr;
		}
		if (advanceFrameBtn != nullptr) {
			advanceFrameBtn->removeFromParent();
			advanceFrameBtn = nullptr;
		}
		if (speedhackBtn != nullptr) {
			speedhackBtn->removeFromParent();
			speedhackBtn = nullptr;
		}
		if (buttonsMenu != nullptr) {
			buttonsMenu->removeFromParent();
			buttonsMenu = nullptr;
		}
	}

	releaseKeys();

	frameLabel = nullptr;
	stateLabel = nullptr;

	androidAction = nullptr;
	leftOver = 0.f;

	if (PlayLayer::get()) {
		CCArray* children = PlayLayer::get()->getChildren();
		CCObject* child;
		CCARRAY_FOREACH(children, child) {
    		CCLabelBMFont* lbl = dynamic_cast<CCLabelBMFont*>(child);
    		if (lbl) {
				if (lbl->getID() == "frameLabel" || lbl->getID() == "stateLabel") lbl->removeFromParentAndCleanup(true);
   			}
		}
	}
	
	mod->setSettingValue("frame_stepper", false);
	if (!safeMode) {
		playedMacro = false;
		if (!isAndroid && mod->getSettingValue<bool>("auto_safe_mode")) {
			safeModeEnabled = false;
			safeMode::updateSafeMode();
		}
	}
}

class mobileButtons {
public:
	void frameAdvance(CCObject*) {
	if (!mod->getSettingValue<bool>("disable_frame_stepper")) {
		if (mod->getSettingValue<bool>("frame_stepper")) stepFrame = true;
		else {
			mod->setSettingValue("frame_stepper", true);
			if (disableFSBtn == nullptr)  {
				int y = 35;
				if (PlayLayer::get()->m_levelSettings->m_platformerMode) y = 95;

				auto winSize = CCDirector::sharedDirector()->getWinSize();
				CCSprite* spr = nullptr;
				CCMenuItemSpriteExtra* btn = nullptr;
				spr = CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png");
				spr->setOpacity(102);
        		spr->setScale(0.8f);
				btn = CCMenuItemSpriteExtra::create(
        		spr,
        		PlayLayer::get(),
				menu_selector(mobileButtons::disableFrameStepper)
    			);
				btn->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(70, y));
				btn->setZOrder(100);
				btn->setID("disable_fs_btn");
				buttonsMenu->addChild(btn);
				disableFSBtn = btn;
			}
		} 
	}
}

void toggleSpeedhack(CCObject*) {
	if (!mod->getSettingValue<bool>("disable_speedhack")) {
		if (prevSpeed != 1 && mod->getSettingValue<double>("speedhack") == 1)
			mod->setSettingValue("speedhack", prevSpeed);
		else {
			prevSpeed = mod->getSettingValue<double>("speedhack");
			mod->setSavedValue<float>("previous_speed", prevSpeed);
			mod->setSettingValue("speedhack", 1.0);
		}
	}
}

void disableFrameStepper(CCObject*) {
	if (mod->getSettingValue<bool>("frame_stepper")) {
		recorder.syncMusic();
		mod->setSettingValue("frame_stepper", false);
		if (disableFSBtn != nullptr) {
			disableFSBtn->removeFromParent();
			disableFSBtn = nullptr;
		}
	}
}

};

void addButton(const char* id) {
	auto winSize = CCDirector::sharedDirector()->getWinSize();
	CCSprite* spr = nullptr;
	CCMenuItemSpriteExtra* btn = nullptr;

	int y = 35;
	if (PlayLayer::get()->m_levelSettings->m_platformerMode) y = 95;
		
	if (id == "advance_frame_btn") {
		spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        spr->setScale(0.8f);
		spr->setOpacity(102);
        auto icon = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
        icon->setPosition(spr->getContentSize() / 2 + ccp(2.5f,0));
        icon->setScaleY(0.7f);
        icon->setScaleX(-0.7f);
		icon->setOpacity(102);
        spr->addChild(icon);
		btn = CCMenuItemSpriteExtra::create(
        	spr,
        	PlayLayer::get(),
			menu_selector(mobileButtons::frameAdvance)
    	);
		btn->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(20, y));
		btn->setID(id);
		btn->setZOrder(100);
		buttonsMenu->addChild(btn);
		advanceFrameBtn = btn;
	} else if (id == "speedhack_btn") {
		spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        spr->setScale(0.8f);
		spr->setOpacity(102);
        auto icon = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
        icon->setPosition(spr->getContentSize() / 2);
		icon->setOpacity(102);
        spr->addChild(icon);
		btn = CCMenuItemSpriteExtra::create(
        	spr,
        	PlayLayer::get(),
			menu_selector(mobileButtons::toggleSpeedhack)
    	);
		btn->setPosition(winSize/2 + ccp(winSize.width/2, -winSize.height/2) + ccp(-20, 35));
		btn->setID(id);
		btn->setZOrder(100);
		buttonsMenu->addChild(btn);
		speedhackBtn = btn;
	} else if (id == "disable_fs_btn") {
		spr = CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png");
		spr->setOpacity(102);
        spr->setScale(0.8f);
		btn = CCMenuItemSpriteExtra::create(
        	spr,
        	PlayLayer::get(),
			menu_selector(mobileButtons::disableFrameStepper)
    	);
		btn->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(70, y));
		btn->setID(id);
		btn->setZOrder(100);
		buttonsMenu->addChild(btn);
		disableFSBtn = btn;
	}
}

void addLabel(const char* text) {
	auto label = CCLabelBMFont::create(text, "chatFont.fnt");
	auto winSize = CCDirector::sharedDirector()->getWinSize();
	label->setScale(0.7f);
	if (text != "Frame: 0") {
		stateLabel = label;
		label->setID("stateLabel");
		label->setPosition(winSize/2 + ccp(winSize.width/2, -winSize.height/2) + ccp(-31, 12));
	} else {
		label->setAnchorPoint(ccp(0.0f,0.5f));
		label->setID("frameLabel");
		frameLabel = label;
		label->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(6, 12));
	}
	label->setZOrder(100);
	PlayLayer::get()->addChild(label);
}

void checkUI() {
	if (recorder.state == state::off) {
		if (frameLabel != nullptr) {
			frameLabel->removeFromParent();
			frameLabel = nullptr;
		}
		if (stateLabel != nullptr) {
			stateLabel->removeFromParent();
			stateLabel = nullptr;
		}
	} else {
		if (frameLabel != nullptr) {
		if (!mod->getSettingValue<bool>("show_frame_label")) {
			frameLabel->removeFromParent();
			frameLabel = nullptr;
		}
		} else if (mod->getSettingValue<bool>("show_frame_label"))
			addLabel("Frame: 0");
	}

		if (recorder.state != state::recording && isAndroid) {
			if (disableFSBtn != nullptr) {
				disableFSBtn->removeFromParent();
				disableFSBtn = nullptr;
			}
			if (advanceFrameBtn != nullptr) {
				advanceFrameBtn->removeFromParent();
				advanceFrameBtn = nullptr;
			}
			if (speedhackBtn != nullptr) {
				speedhackBtn->removeFromParent();
				speedhackBtn = nullptr;
			}
			if (buttonsMenu != nullptr) {
				buttonsMenu->removeFromParent();
				buttonsMenu = nullptr;
			}
		} else if (recorder.state == state::recording && isAndroid) {
			if (buttonsMenu != nullptr) {
			if (advanceFrameBtn != nullptr) {
				if (mod->getSettingValue<bool>("disable_frame_stepper")) {
					if (disableFSBtn != nullptr) {
						disableFSBtn->removeFromParent();
						disableFSBtn = nullptr;
					}
					advanceFrameBtn->removeFromParent();
					advanceFrameBtn = nullptr;
				}
			} else if (!mod->getSettingValue<bool>("disable_frame_stepper")) {
				addButton("advance_frame_btn");
				if (mod->getSettingValue<bool>("frame_stepper")){
					addButton("disable_fs_btn");
				}
			}

			if (speedhackBtn != nullptr) {
				if (mod->getSettingValue<bool>("disable_speedhack")) {
					speedhackBtn->removeFromParent();
					speedhackBtn = nullptr;
				}
			} else if (!mod->getSettingValue<bool>("disable_speedhack"))
				addButton("speedhack_btn");

			} else {
			if (!mod->getSettingValue<bool>("disable_frame_stepper")) {
				buttonsMenu = CCMenu::create();
				buttonsMenu->setPosition({0,0});
				PlayLayer::get()->addChild(buttonsMenu);
				addButton("advance_frame_btn");
				if (mod->getSettingValue<bool>("frame_stepper")){
					addButton("disable_fs_btn");
				}
			}
			if (!mod->getSettingValue<bool>("disable_speedhack")) {
				if (buttonsMenu == nullptr) {
					buttonsMenu = CCMenu::create();
					buttonsMenu->setPosition({0,0});
					PlayLayer::get()->addChild(buttonsMenu);
				}
				addButton("speedhack_btn");
			}
			}
		}

		if (recorder.state == state::playing) {
			if (stateLabel != nullptr) {
				if (stateLabel->getString() != "Playing" && mod->getSettingValue<bool>("show_playing_label"))
					stateLabel->setString("Playing");
				else if (!mod->getSettingValue<bool>("show_playing_label")) {
					stateLabel->removeFromParent();
					stateLabel = nullptr;
				}
			} else if (mod->getSettingValue<bool>("show_playing_label")) {
				addLabel("Playing");
			}
		}
		if (recorder.state == state::recording) {
			if (stateLabel != nullptr) {
				if (stateLabel->getString() != "Recording" && mod->getSettingValue<bool>("show_recording_label"))
					stateLabel->setString("Recording");
				else if (!mod->getSettingValue<bool>("show_recording_label")) {
					stateLabel->removeFromParent();
					stateLabel = nullptr;
				}
			} else if (mod->getSettingValue<bool>("show_recording_label")) {
				addLabel("Recording");
			}
		}
}
void onReset() {
	if (recorder.state != state::off && restart != false) {
			restart = false;
		}
		
		checkUI();
		playerHolding = false;
		leftOver = 0.f;

		if (isAndroid) androidAction = nullptr;

		if (safeModeEnabled && !isAndroid && mod->getSettingValue<bool>("auto_safe_mode")) {
			safeModeEnabled = false;
			safeMode::updateSafeMode();
		}
		
		if (playedMacro) playedMacro = false;


		if (recorder.state == state::playing) {
			playingAction = false;
			releaseKeys();
			recorder.currentAction = 0;
			if (mod->getSettingValue<bool>("speedhack_audio")) {
				FMOD::ChannelGroup* channel;
        		FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        		channel->setPitch(1);
			}
		}
}
	// ---------------- Hooks ---------------- 539//

class $modify(PlayerObject) {
void playerDestroyed(bool p0) {
	if (isAndroid) androidAction = nullptr;
	if  (isAndroid && mod->getSettingValue<bool>("auto_safe_mode") && playedMacro) {
		noDelayedReset = true;
		onReset();
		return PlayLayer::get()->resetLevel();
	}
	if (!mod->getSettingValue<bool>("instant_respawn") || recorder.state == state::off)
		return PlayerObject::playerDestroyed(p0);
	onReset();
	return PlayLayer::get()->resetLevel();
}
void playDeathEffect() {
	if (!mod->getSettingValue<bool>("disable_death_effect") || recorder.state == state::off)
		PlayerObject::playDeathEffect();
}
};

class $modify(PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		CCSprite* sprite = nullptr;

		if (Loader::get()->isModLoaded("tpdea.betterpause-Better")) {
			sprite = CCSprite::createWithSpriteFrameName("GJ_stopEditorBtn_001.png");
		} else {
 			sprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
			sprite->setScale(0.35f);
		}
       

        auto btn = CCMenuItemSpriteExtra::create(sprite,
		this,
		menu_selector(RecordLayer::openMenu));

		auto menu = this->getChildByID("right-button-menu");
        menu->addChild(btn);
		menu->updateLayout();
	} 

	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		clearState(false);
	}

	void goEdit() {
		PauseLayer::goEdit();
		clearState(false);
	}

	void onResume(CCObject* sender) {
		PauseLayer::onResume(sender);
		checkUI();
		if (restart) PlayLayer::get()->resetLevel();
		if (recorder.state == state::off) {
			if (mod->getSettingValue<bool>("speedhack_audio")) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
			channel->setPitch(1);
			}
		}
	}

	void onPracticeMode(CCObject* sender) {
		PauseLayer::onPracticeMode(sender);
		checkUI();
		if (restart) PlayLayer::get()->resetLevel();
		if (recorder.state == state::off) {
			if (mod->getSettingValue<bool>("speedhack_audio")) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
			channel->setPitch(1);
			}
		}
	}

};

class $modify(GJBaseGameLayer) {
	void toggleFlipped(bool p0, bool p1) {
		if (!mod->getSettingValue<bool>("instant_mirror") || recorder.state == state::off)
       		return GJBaseGameLayer::toggleFlipped(p0, p1);
		return GJBaseGameLayer::toggleFlipped(p0, true);
    }
	void handleButton(bool holding, int button, bool player1) {
		if (!isAndroid) {
			if ((recorder.state == state::playing && (playingAction || !mod->getSettingValue<bool>("ignore_inputs"))) 
			|| recorder.state != state::playing) GJBaseGameLayer::handleButton(holding,button,player1);
		}
		if (isAndroid) {
			if (recorder.state == state::recording) {
			GJBaseGameLayer::handleButton(holding,button,player1);
			playerData p1;
			playerData p2;
				p1 = {
				this->m_player1->getPositionX(),
				this->m_player1->getPositionY(),
				this->m_player1->m_isUpsideDown,
				-80085,
				-80085,
				-80085
			};
			if (this->m_player2 != nullptr) {
				p2 = {
				this->m_player2->getPositionX(),
				this->m_player2->getPositionY(),
				this->m_player2->m_isUpsideDown,
				-80085,
				-80085,
				-80085
				};
			} else {
				p2.xPos = 0;
			}
			int frame = recorder.currentFrame(); 
			recorder.recordAction(holding, button, player1, frame, this, p1, p2);
		} else if (recorder.state == state::playing) {
			GJBaseGameLayer::handleButton(holding,button,player1);
			if (androidAction != nullptr) {
			if (androidAction->p1.xPos != 0) {
				if (!areEqual(this->m_player1->getPositionX(), androidAction->p1.xPos) ||
				!areEqual(this->m_player1->getPositionY(), androidAction->p1.yPos))
					this->m_player1->setPosition(cocos2d::CCPoint(androidAction->p1.xPos, androidAction->p1.yPos));
					
				if (androidAction->p2.xPos != 0 && this->m_player2 != nullptr) {
					if (!areEqual(this->m_player2->getPositionX(), androidAction->p2.xPos) ||
					!areEqual(this->m_player2->getPositionY(), androidAction->p2.yPos))
						this->m_player2->setPosition(cocos2d::CCPoint(androidAction->p2.xPos, androidAction->p2.yPos));

				}
			}
		}
		} else GJBaseGameLayer::handleButton(holding,button,player1);

	} else if (recorder.state == state::recording) {
			playerData p1;
			playerData p2;
			if (!mod->getSettingValue<bool>("vanilla") || mod->getSettingValue<bool>("frame_fix")) {
				if (!mod->getSettingValue<bool>("frame_fix")) playerHolding = holding;
				if (!recorder.macro.empty()) {
					try {
						if (recorder.macro.back().frame == recorder.currentFrame() && recorder.macro.back().posOnly) {
							recorder.macro.pop_back();
						}
					} catch (const std::exception& e) {
						log::debug("wtfffff amaze real? - {}",e);
					}
				}
				p1 = {
				this->m_player1->getPositionX(),
				this->m_player1->getPositionY(),
				this->m_player1->m_isUpsideDown,
				-80085,
				-80085,
				-80085
			};
			if (this->m_player2 != nullptr) {
				p2 = {
				this->m_player2->getPositionX(),
				this->m_player2->getPositionY(),
				this->m_player2->m_isUpsideDown,
				-80085,
				-80085,
				-80085
				};
			} else {
				p2.xPos = 0;
			}
			} else {
				p1.xPos = 0;
			}
			if (mod->getSettingValue<bool>("vanilla") && !mod->getSettingValue<bool>("frame_fix")) {
				p1 = {
				0.f,
				0.f,
				false,
				-80085,
				-80085,
				-80085
			};
				p2 = {
				0.f,
				0.f,
				false,
				-80085,
				-80085,
				-80085
				};
		}
			recorder.recordAction(holding, button, player1, recorder.currentFrame(), this, p1, p2);
}
	}

	int getPlayer1(int p1, GJBaseGameLayer* bgl) {
		bool player1;
		if (GameManager::get()->getGameVariable("0010") && !bgl->m_levelSettings->m_platformerMode && bgl->m_levelSettings->m_twoPlayerMode) player1 = !p1;
		else player1 = p1;
		return static_cast<int>(player1);
	}

	void update(float dt) {
		if (recorder.state != state::off) {
			if (frameLabel != nullptr)
					frameLabel->setString(("Frame: " + std::to_string(recorder.currentFrame())).c_str());
		} else if (shouldPlay) {
			if (recorder.currentFrame() == 0) {
				shouldPlay = false;
				recorder.state = state::playing;
				PlayLayer::get()->resetLevel();
			}
		}

		if (recorder.state == state::recording) {
			if (mod->getSettingValue<bool>("frame_stepper") && stepFrame == false) 
				return;
			else if (stepFrame) {
				int fps = (recorder.fps > 240) ? 240 : recorder.fps;
				GJBaseGameLayer::update(1.f / fps);
				stepFrame = false;
				recorder.syncMusic();
			} else GJBaseGameLayer::update(dt);
		} else GJBaseGameLayer::update(dt);
		
if (recorder.state == state::playing && isAndroid) {
			int frame = recorder.currentFrame();
        	while (recorder.currentAction < static_cast<int>(recorder.macro.size()) &&
			frame >= recorder.macro[recorder.currentAction].frame && !this->m_player1->m_isDead) {

            	auto& currentActionIndex = recorder.macro[recorder.currentAction];
				androidAction = &currentActionIndex;
				
				if (!currentActionIndex.posOnly) {
					int player = (this->m_levelSettings->m_twoPlayerMode)
					? getPlayer1(currentActionIndex.player1, this)
					: 0;

					if (!playedMacro) playedMacro = true;
					int pBtn = (currentActionIndex.button < 4 && currentActionIndex.button > 0) ? currentActionIndex.button : 1;

					cocos2d::CCKeyboardDispatcher::get()->dispatchKeyboardMSG(
					static_cast<cocos2d::enumKeyCodes>(playerEnums[player]
					[pBtn-1]),
					currentActionIndex.holding, false);

				}
            	recorder.currentAction++;
        	}
			if (recorder.currentAction >= recorder.macro.size()) {
				if (stateLabel!=nullptr) stateLabel->removeFromParent();
				clearState(true);
			}
		}

	}
};

void GJBaseGameLayerProcessCommands(GJBaseGameLayer* self) {
	reinterpret_cast<void(__thiscall *)(GJBaseGameLayer *)>(base::get() + 0x1BD240)(self);
	if (recorder.state == state::recording) {
		if (((playerHolding && !mod->getSettingValue<bool>("vanilla")) ||
		mod->getSettingValue<bool>("frame_fix")) && !recorder.macro.empty()) {
			
			if (!(recorder.macro.back().frame == recorder.currentFrame() &&
			(recorder.macro.back().posOnly || recorder.macro.back().p1.xPos != 0))) {
				playerData p1 = {
					self->m_player1->getPositionX(),
					self->m_player1->getPositionY(),
					self->m_player1->m_isUpsideDown,
					-80085,
					-80085,
					-80085
				};
				playerData p2;
				if (self->m_player2 != nullptr) {
					p2 = {
					self->m_player2->getPositionX(),
					self->m_player2->getPositionY(),
					self->m_player2->m_isUpsideDown,
					-80085,
					-80085,
					-80085
					};
				} else {
					p2.xPos = 0;
				}
				recorder.macro.push_back({true,recorder.currentFrame(),1,true,true,p1,p2});
			}
		}
	}

	if (recorder.state == state::playing) {
			int frame = recorder.currentFrame();
        	while (recorder.currentAction < static_cast<int>(recorder.macro.size()) &&
			frame >= recorder.macro[recorder.currentAction].frame && !self->m_player1->m_isDead) {
            	auto& currentActionIndex = recorder.macro[recorder.currentAction];

				if (!safeModeEnabled && !isAndroid && mod->getSettingValue<bool>("auto_safe_mode")) {
					safeModeEnabled = true;
					safeMode::updateSafeMode();
				}
				playedMacro = true;
				if (!mod->getSettingValue<bool>("override_macro_mode") && currentActionIndex.p1.xPos != 0) {
						if (!areEqual(self->m_player1->getPositionX(), currentActionIndex.p1.xPos) ||
						!areEqual(self->m_player1->getPositionY(), currentActionIndex.p1.yPos))
								self->m_player1->setPosition(cocos2d::CCPoint(currentActionIndex.p1.xPos, currentActionIndex.p1.yPos));

						if (self->m_player1->m_isUpsideDown != currentActionIndex.p1.upsideDown && currentActionIndex.posOnly)
							self->m_player1->flipGravity(currentActionIndex.p1.upsideDown, true);

					
						if (currentActionIndex.p2.xPos != 0 && self->m_player2 != nullptr) {
							if (!areEqual(self->m_player2->getPositionX(), currentActionIndex.p2.xPos) ||
							!areEqual(self->m_player2->getPositionY(), currentActionIndex.p2.yPos))
								self->m_player2->setPosition(cocos2d::CCPoint(currentActionIndex.p2.xPos, currentActionIndex.p2.yPos));

							if (self->m_player2->m_isUpsideDown != currentActionIndex.p2.upsideDown && currentActionIndex.posOnly)
								self->m_player2->flipGravity(currentActionIndex.p2.upsideDown, true);

						}
				} else {
				if ((currentActionIndex.p1.xPos != 0 && self->m_player1 != nullptr) && (!mod->getSettingValue<bool>("vanilla") || mod->getSettingValue<bool>("frame_fix"))) {
					if (((!mod->getSettingValue<bool>("vanilla") && !mod->getSettingValue<bool>("frame_fix")) && lastHold)
					|| mod->getSettingValue<bool>("frame_fix")) {
						if (!areEqual(self->m_player1->getPositionX(), currentActionIndex.p1.xPos) ||
						!areEqual(self->m_player1->getPositionY(), currentActionIndex.p1.yPos))
							self->m_player1->setPosition(cocos2d::CCPoint(currentActionIndex.p1.xPos, currentActionIndex.p1.yPos));
							

						if (self->m_player1->m_isUpsideDown != currentActionIndex.p1.upsideDown && currentActionIndex.posOnly)
							self->m_player1->flipGravity(currentActionIndex.p1.upsideDown, true);

					
						if (currentActionIndex.p2.xPos != 0 && self->m_player2 != nullptr) {
							if (!areEqual(self->m_player2->getPositionX(), currentActionIndex.p2.xPos) ||
							!areEqual(self->m_player2->getPositionY(), currentActionIndex.p2.yPos))
								self->m_player2->setPosition(cocos2d::CCPoint(currentActionIndex.p2.xPos, currentActionIndex.p2.yPos));

							if (self->m_player2->m_isUpsideDown != currentActionIndex.p2.upsideDown && currentActionIndex.posOnly)
								self->m_player2->flipGravity(currentActionIndex.p2.upsideDown, true);

						}
					}
				}
				}
				if (!currentActionIndex.posOnly) {
					playingAction = true;

					self->handleButton(
    				currentActionIndex.holding,
    				((currentActionIndex.button < 4 && currentActionIndex.button > 0) ? currentActionIndex.button : 1),
    				currentActionIndex.player1
					);

					if (currentActionIndex.holding) lastHold = true;
					else lastHold = false;
				} else if (currentActionIndex.p1.xPos == 0) {
					playingAction = true;

					self->handleButton(
    				false,
    				1,
    				true
					);

					if (currentActionIndex.holding) lastHold = true;
					else lastHold = false;
				}

            	recorder.currentAction++;
        	}
			playingAction = false;
			if (recorder.currentAction >= recorder.macro.size()) {
				if (stateLabel!=nullptr) stateLabel->removeFromParent();
				clearState(true);
			}
		}
}

class $modify(PlayLayer) {
	void destroyPlayer(PlayerObject* p1, GameObject* p2) {
		if (!mod->getSettingValue<bool>("noclip") || recorder.state == state::off)
			PlayLayer::destroyPlayer(p1,p2);
	}

	void delayedResetLevel() {
		if (noDelayedReset) {
			noDelayedReset = false;
			return;	
		} 
		if (!mod->getSettingValue<bool>("instant_respawn") || recorder.state == state::off)
			PlayLayer::delayedResetLevel();
	}
	void resetLevel() {
		checkUI();
		PlayLayer::resetLevel();
		if (recorder.state != state::off && restart != false) {
			restart = false;
		}
		
		if (recorder.state != state::off) {
			playerHolding = false;
			leftOver = 0.f;
		}

		if (isAndroid) androidAction = nullptr;

		if (safeModeEnabled && !isAndroid && mod->getSettingValue<bool>("auto_safe_mode")) {
			safeModeEnabled = false;
			safeMode::updateSafeMode();
		}
		
		if (playedMacro) playedMacro = false;


		if (recorder.state == state::playing) {
			releaseKeys();
			playingAction = false;
			recorder.currentAction = 0;
			if (mod->getSettingValue<bool>("speedhack_audio")) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        	channel->setPitch(1);
			}
		} else if (recorder.state != state::off) {
			checkUI();
        	if (this->m_isPracticeMode && !recorder.macro.empty() && recorder.currentFrame() != 0) {
  				int frame = recorder.currentFrame(); 
				try {
            	if (!recorder.macro.empty()) {
						for (auto it = recorder.macro.rbegin(); it != recorder.macro.rend(); ++it) {
        					if (it->frame >= frame) {
								try {
									recorder.macro.erase((it + 1).base());
								} catch (const std::exception& e) {
									log::debug("wtfffff amaze? - {}",e);
								}
							} else break;
    					}
						bool fix = false;
					if (recorder.macro.size() >= 2) {
							if (recorder.macro.back().holding || (recorder.macro[recorder.macro.size() - 2].holding && !recorder.macro[recorder.macro.size() - 2].player1))
								fix = true;
						} else if (recorder.macro.back().holding)
							fix = true;

						if (fix) {
						playerData p1;
						playerData p2;
						p1 = {
							0.f,
							0.f,
							false,
							-80085,
							-80085,
							-80085
						};
						p2 = {
							0.f,
							0.f,
							false,
							-80085,
							-80085,
							-80085
						};
						recorder.macro.push_back({false, frame, 1, false, false, p1, p2});
						recorder.macro.push_back({true, frame, 1, false, false, p1, p2});
						if (this->m_levelSettings->m_platformerMode) {
							recorder.macro.push_back({false, frame, 2, false, false, p1, p2});
							recorder.macro.push_back({true, frame, 2, false, false, p1, p2});
							recorder.macro.push_back({false, frame, 3, false, false, p1, p2});
							recorder.macro.push_back({true, frame, 3, false, false, p1, p2});
						}
					}
				}
				} catch (const std::exception& e) {
					log::debug("wtfffff? - {}",e);
				}
        	} else {
				if (!recorder.macro.empty())
					recorder.macro.clear();

				recorder.android = false;
				recorder.fps = fpsArr[fpsIndex];
			} 
   		}
	}

	void levelComplete() {
		if (stateLabel != nullptr) stateLabel->removeFromParent();
		if (isAndroid && mod->getSettingValue<bool>("auto_safe_mode") && playedMacro) {
			PlayLayer::get()->m_isTestMode = true;
			return;
		}
		PlayLayer::levelComplete();
		if (recorder.state != state::off)
			shouldPlay2 = true;
		
		clearState(true);
	}
};

class $modify(EndLevelLayer) {

	void customSetup() {
		EndLevelLayer::customSetup();
		if (mod->getSettingValue<bool>("end_button")) {
			auto winSize = CCDirector::sharedDirector()->getWinSize();
			CCSprite* sprite = nullptr;
			sprite = CCSprite::createWithSpriteFrameName("GJ_playBtn2_001.png");
			sprite->setScale(0.350f);
        	auto btn = CCMenuItemSpriteExtra::create(sprite,
			this,
			menu_selector(RecordLayer::openMenu));

			btn->setPosition(winSize/2 + CCPOINT_CREATE(160, -99));

			auto layer = reinterpret_cast<CCLayer*>(this->getChildren()->objectAtIndex(0));
			auto menu = CCMenu::create();
			menu->setPosition({0,0});
        	menu->addChild(btn);
			layer->addChild(menu);
		}
		if ((!mod->getSettingValue<bool>("auto_safe_mode")) && playedMacro) {
			auto winSize = CCDirector::sharedDirector()->getWinSize();
			auto layer = reinterpret_cast<CCLayer*>(this->getChildren()->objectAtIndex(0));
			auto watermarkxd = CCLabelBMFont::create("Recorded with xdBot.", "chatFont.fnt");
			watermarkxd->setOpacity(60);
			watermarkxd->setAnchorPoint(ccp(0.0f,0.5f));
			watermarkxd->setPosition(winSize/2 + ccp(-winSize.width/2, -winSize.height/2) + ccp(3, 6));
			watermarkxd->setScale(0.5f);
			layer->addChild(watermarkxd);
		}
	}

	void onReplay(CCObject* s) {
		EndLevelLayer::onReplay(s);
		if (shouldPlay2 && mod->getSettingValue<bool>("auto_enable_play")) {
			shouldPlay2 = false;
			shouldPlay = true;
		}
		clearState(false);
	}

	void goEdit() {
		EndLevelLayer::goEdit();
		clearState(false);
	}

	void onMenu(CCObject* s) {
		EndLevelLayer::onMenu(s);
		clearState(false);
	}
};
int syncCooldown = 0;
int holdCooldown = 0;
class $modify(CCScheduler) {
	void update(float dt) {
		if (recorder.state == state::off) return CCScheduler::update(dt);

		if (holdV) holdCooldown++;
		if (holdCooldown > 60) {
			if (!mod->getSettingValue<bool>("disable_frame_stepper")) {
				if (mod->getSettingValue<bool>("frame_stepper")) stepFrame = true;
				else {
					mod->setSettingValue("frame_stepper", true);
					if (disableFSBtn == nullptr && isAndroid) 
						addButton("disable_fs_btn");
				} 
			}
		}

		std::stringstream ss;
    	ss << std::fixed << std::setprecision(2) << static_cast<float>(mod->getSettingValue<double>("speedhack"));
    	float speedhackValue = std::stof(ss.str());

		if (mod->getSettingValue<bool>("speedhack_audio")) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        	channel->setPitch(speedhackValue);
		}

		using namespace std::literals;

		float dt2 = (1.f / recorder.fps);

		dt *= speedhackValue;

    	auto startTime = std::chrono::high_resolution_clock::now();
		int mult = static_cast<int>((dt + leftOver)/dt2);  
    	for (int i = 0; i < mult; ++i) {
        	CCScheduler::update(dt2);
        	if (std::chrono::high_resolution_clock::now() - startTime > 33.333ms) {
            	mult = i + 1;
            	break;
        	}
    	}
    leftOver += (dt - dt2 * mult); 
	if (recorder.state == state::playing && !PlayLayer::get()->m_levelSettings->m_platformerMode) {
		syncCooldown++;
		if (syncCooldown >= 20 && leftOver > 1) {
			syncCooldown = 0;
		}
	}
	}
};

class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool hold, bool p) {
		if (key == cocos2d::enumKeyCodes::KEY_C && hold && !p && recorder.state != state::off) {
			if (!mod->getSettingValue<bool>("disable_speedhack")) {
				if (prevSpeed != 1 && mod->getSettingValue<double>("speedhack") == 1)
					mod->setSettingValue("speedhack", prevSpeed);
				else {
					prevSpeed = mod->getSettingValue<double>("speedhack");
					mod->setSavedValue<float>("previous_speed", prevSpeed);
					mod->setSettingValue("speedhack", 1.0);
				}
			}
		}

		if (key == cocos2d::enumKeyCodes::KEY_V && !p && recorder.state == state::recording) {
			if (!mod->getSettingValue<bool>("disable_frame_stepper") && hold) {
				if (mod->getSettingValue<bool>("frame_stepper")) stepFrame = true;
				else {
					mod->setSettingValue("frame_stepper", true);
					if (disableFSBtn == nullptr && isAndroid) 
						addButton("disable_fs_btn");
				} 
			} else if (!hold) holdCooldown = 0;
			holdV = hold;
		}

		if (key == cocos2d::enumKeyCodes::KEY_B && hold && !p && recorder.state == state::recording) {
			if (mod->getSettingValue<bool>("frame_stepper")) {
				recorder.syncMusic();
				mod->setSettingValue("frame_stepper", false);
				if (disableFSBtn != nullptr) {
					disableFSBtn->removeFromParent();
					disableFSBtn = nullptr;
				}
			}
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key,hold,p);
	}
};

$execute {
	mod = Mod::get();
	if (mod->getSavedValue<float>("previous_fps"))
		fpsIndex = mod->getSavedValue<float>("previous_fps");
	else if (isAndroid)
		fpsIndex = 0;
	else
		fpsIndex = 3;

	recorder.fps = fpsArr[fpsIndex];

	if (mod->getSavedValue<float>("previous_speed"))
		prevSpeed = mod->getSavedValue<float>("previous_speed");
	else
		prevSpeed = 0.5f;

	if (!isAndroid)
		mod->hook(reinterpret_cast<void *>(base::get() + 0x1BD240), &GJBaseGameLayerProcessCommands, "GJBaseGameLayer::processCommands", tulip::hook::TulipConvention::Thiscall);
	else {
		if (sizeof(void*) == 8) {
        	offset = 0x3B8;
    	}
	}

	for (std::size_t i = 0; i < 15; i++) {
		safeMode::patches[i] = mod->patch(reinterpret_cast<void*>(base::get() + std::get<0>(safeMode::codes[i])),
		std::get<1>(safeMode::codes[i])).unwrap();
		safeMode::patches[i]->disable();
	}
}
