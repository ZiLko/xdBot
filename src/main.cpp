#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
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

float leftOver = 0.f; // For CCScheduler
int fixedFps = 240;
bool restart = false;
bool stepFrame = false;
double prevSpeed = 1.0f;
bool safeModeEnabled = false;
bool playerHolding = false;
bool lastHold = false;
bool shouldPlay = false;
bool shouldPlay2 = false;

CCLabelBMFont* frameLabel = nullptr;
CCLabelBMFont* stateLabel = nullptr;

using namespace geode::prelude;



namespace safeMode
{
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
		if (safeModeEnabled) {
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
	double xPos;
	double yPos;
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

enum state {
    off,
    recording,
    playing
};

class recordSystem {
public:
    state state = off;
 	size_t currentAction = 0;
   	std::vector<data> macro;

	int currentFrame() {
		return static_cast<int>((*(double*)(((char*)PlayLayer::get()) + 0x328)) * fixedFps); // m_time * fps
	}
	void syncMusic() {
		FMODAudioEngine::sharedEngine()->setMusicTimeMS(
			(currentFrame()*1000)/240 + PlayLayer::get()->m_levelSettings->m_songOffset*1000,
			true,
			0
		);
	}
	void recordAction(bool holding, int button, bool player1, int frame, GJBaseGameLayer* bgl, playerData p1Data, playerData p2Data) {
    	macro.push_back({player1, frame, button, holding, false, p1Data, p2Data});
	}

};

recordSystem recorder;

class RecordLayer : public geode::Popup<std::string const&> {
 	CCLabelBMFont* infoMacro = nullptr;
 	CCMenuItemToggler* recording = nullptr;
    CCMenuItemToggler* playing = nullptr;
protected:
    bool setup(std::string const& value) override {
        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
		auto versionLabel = CCLabelBMFont::create("xdBot v1.3.7 - made by Zilko", "chatFont.fnt");
		versionLabel->setOpacity(60);
		versionLabel->setAnchorPoint(CCPOINT_CREATE(0.0f,0.5f));
		versionLabel->setPosition(winSize/2 + CCPOINT_CREATE(-winSize.width/2, -winSize.height/2) + CCPOINT_CREATE(3, 6));
		versionLabel->setScale(0.5f);
		this->addChild(versionLabel);
		this->setTitle("xdBot");
		auto menu = CCMenu::create();
    	menu->setPosition({0, 0});
    	m_mainLayer->addChild(menu);

 		auto checkOffSprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
   		auto checkOnSprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

		CCPoint topLeftCorner = winSize/2.f-CCPOINT_CREATE(m_size.width/2.f,-m_size.height/2.f);

		auto label = CCLabelBMFont::create("Record", "bigFont.fnt"); 
    	label->setAnchorPoint({0, 0.5});
    	label->setScale(0.7f);
    	label->setPosition(topLeftCorner + CCPOINT_CREATE(168, -60));
    	m_mainLayer->addChild(label);

    	recording = CCMenuItemToggler::create(checkOffSprite,
		checkOnSprite,
		this,
		menu_selector(RecordLayer::toggleRecord));

    	recording->setPosition(label->getPosition() + CCPOINT_CREATE(105,0));
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
    	btn->setPosition(winSize/2.f-CCPOINT_CREATE(m_size.width/2.f,m_size.height/2.f) + CCPOINT_CREATE(325, 20));
    	menu->addChild(btn);

		spr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    	spr->setScale(0.65f);
    	btn = CCMenuItemSpriteExtra::create(
        	spr,
        	this,
        	menu_selector(RecordLayer::keyInfo)
    	);
    	btn->setPosition(topLeftCorner + CCPOINT_CREATE(290, -10));
    	menu->addChild(btn);

    	label = CCLabelBMFont::create("Play", "bigFont.fnt");
    	label->setScale(0.7f);
    	label->setPosition(topLeftCorner + CCPOINT_CREATE(198, -90)); 
    	label->setAnchorPoint({0, 0.5});
    	m_mainLayer->addChild(label);

     	playing = CCMenuItemToggler::create(checkOffSprite, checkOnSprite,
	 	this,
	 	menu_selector(RecordLayer::togglePlay));

    	playing->setPosition(label->getPosition() + CCPOINT_CREATE(75,0)); 
    	playing->setScale(0.85f);
    	playing->toggle(recorder.state == state::playing); 
    	menu->addChild(playing);

 		auto btnSprite = ButtonSprite::create("Save");
    	btnSprite->setScale(0.72f);

   		btn = CCMenuItemSpriteExtra::create(btnSprite,
   		this,
   		menu_selector(saveMacroPopup::openSaveMacro));

    	btn->setPosition(topLeftCorner + CCPOINT_CREATE(65, -160)); 
    	menu->addChild(btn);

		btnSprite = ButtonSprite::create("Load");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(loadMacroPopup::openLoadMenu));

    	btn->setPosition(topLeftCorner + CCPOINT_CREATE(144, -160));
    	menu->addChild(btn);

  		btnSprite = ButtonSprite::create("Clear");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(RecordLayer::clearMacro));

    	btn->setPosition(topLeftCorner + CCPOINT_CREATE(228, -160));
    	menu->addChild(btn);

		infoMacro = CCLabelBMFont::create("", "chatFont.fnt");
    	infoMacro->setAnchorPoint({0, 1});
    	infoMacro->setPosition(topLeftCorner + CCPOINT_CREATE(21, -45));
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
		geode::openSettingsPopup(Mod::get());
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
    	infoText << "\nSize: " << recorder.macro.size();
		infoText << "\nClicks: " << clicksCount;
		infoText << "\nDuration: " << (!recorder.macro.empty() 
		? recorder.macro.back().frame / fixedFps : 0) << "s";
    	infoMacro->setString(infoText.str().c_str());
	}

	void togglePlay(CCObject*) {
		if (recorder.state == state::recording) recording->toggle(false);
    	recorder.state = (recorder.state == state::playing) ? state::off : state::playing;

		if (recorder.state == state::playing) restart = true;
		else if (recorder.state == state::off) restart = false;
		recorder.syncMusic();
		Mod::get()->setSettingValue("frame_stepper", false);
	}

	void toggleRecord(CCObject* sender) {
		if(!recorder.macro.empty() && recorder.state != state::recording) {
			geode::createQuickPopup(
    			"Warning",     
    			"This will <cr>clear</c> the current macro.", 
    			"Cancel", "Ok",  
    			[this, sender](auto, bool btn2) {
        			if (!btn2) this->recording->toggle(false);
			 		else {
						recorder.macro.clear();
						this->toggleRecord(sender);
					}
   				}
			);
		} else {
			if (recorder.state == state::playing) this->playing->toggle(false);
    		recorder.state = (recorder.state == state::recording) 
			? state::off : state::recording;
			if (recorder.state == state::recording) {
				restart = true;
				updateInfo();
			} else if (recorder.state == state::off) {
				restart = false;
				recorder.syncMusic();
				Mod::get()->setSettingValue("frame_stepper", false);
			}
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
		layer->m_noElasticity = (static_cast<float>(Mod::get()->getSettingValue<double>("speedhack")) < 1
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
	layer->m_noElasticity = (static_cast<float>(Mod::get()->getSettingValue<double>("speedhack")) < 1
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

	std::string savePath = Mod::get()->getSaveDir().string()
     +"\\"+std::string(macroNameInput->getString()) + ".xd";

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring wideString = converter.from_bytes(savePath);
	std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);

    std::wofstream file(wideString);
    file.imbue(utf8_locale);

	if (file.is_open()) {
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
    	"Macro saved <cg>succesfully</c>.",  
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
	std::string loadPath = Mod::get()->getSaveDir().string()
    +"\\"+static_cast<CCMenuItemSpriteExtra*>(btn)->getID() + ".xd";
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
	while (std::getline(file, line)) {
		std::wistringstream isSS(line);

		playerData p1;
		playerData p2;

		int holding, frame, button, player1, posOnly;
		double p1xPos, p1yPos, p1rotation, p1xSpeed, p1ySpeed;
		double p2xPos, p2yPos, p2rotation, p2xSpeed, p2ySpeed;
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
					(double)p1xPos,
					(double)p1yPos,
					(bool)p1upsideDown,
					(float)p1rotation,
					(double)p1xSpeed,
					(double)p1ySpeed,
				};
				p2 = {
					(double)p2xPos,
					(double)p2yPos,
					(bool)p2upsideDown,
					(float)p2rotation,
					(double)p2xSpeed,
					(double)p2ySpeed,
				};
				recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding, (bool)posOnly, p1, p2});
			}
		} else {
			if (isSS >> frame >> s >> holding >> s >> button >> 
			s >> player1 && s == L'|') {
				p1.xPos = 0;
				recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding, false, p1, p2});
			}
		}
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
	FMOD::ChannelGroup* channel;
    FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
	channel->setPitch(1);
	recorder.state = state::off;
	
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
	frameLabel = nullptr;
	stateLabel = nullptr;
	Mod::get()->setSettingValue("frame_stepper", false);
	if (!safeMode) {
		safeModeEnabled = false;
		safeMode::updateSafeMode();
	}
}

	// ---------------- Hooks ---------------- 539//

class $modify(PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();
		auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_stopEditorBtn_001.png");
        sprite->setScale(0.75f);

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
		if (restart) PlayLayer::get()->resetLevel();
		if (recorder.state == state::off) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
			channel->setPitch(1);
		} else {
			recorder.syncMusic();
		}
	}

	void onPracticeMode(CCObject* sender) {
		PauseLayer::onPracticeMode(sender);
		if (restart) PlayLayer::get()->resetLevel();
		if (recorder.state == state::off) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
			channel->setPitch(1);
		}
	}

};

void addLabel(const char* text) {
	auto label = CCLabelBMFont::create(text, "chatFont.fnt");
	auto winSize = CCDirector::sharedDirector()->getWinSize();
	label->setScale(0.7f);
	if (text != "Frame: 0") {
		stateLabel = label;
		label->setID("stateLabel");
		label->setPosition(winSize/2 + CCPOINT_CREATE(winSize.width/2, -winSize.height/2) + CCPOINT_CREATE(-31, 12));
	} else {
		label->setAnchorPoint(CCPOINT_CREATE(0.0f,0.5f));
		label->setID("frameLabel");
		frameLabel = label;
		label->setPosition(winSize/2 + CCPOINT_CREATE(-winSize.width/2, -winSize.height/2) + CCPOINT_CREATE(6, 12));
	}
	PlayLayer::get()->addChild(label);
}

class $modify(GJBaseGameLayer) {
	void handleButton(bool holding, int button, bool player1) {
		GJBaseGameLayer::handleButton(holding,button,player1);
		if (recorder.state == state::recording) {
			playerData p1;
			playerData p2;
			if (!Mod::get()->getSettingValue<bool>("vanilla") || Mod::get()->getSettingValue<bool>("frame_fix")) {
				if (!Mod::get()->getSettingValue<bool>("frame_fix")) playerHolding = holding;
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
			int frame = recorder.currentFrame(); 
			recorder.recordAction(holding, button, player1, frame, this, p1, p2);
		}
	}


	void update(float dt) {
		if (recorder.state != state::off) {
			if (frameLabel != nullptr) {
				if (Mod::get()->getSettingValue<bool>("show_frame_label"))
					frameLabel->setString(("Frame: " + std::to_string(recorder.currentFrame())).c_str());
				else {
					frameLabel->removeFromParent();
					frameLabel = nullptr;
				}
			} else if (Mod::get()->getSettingValue<bool>("show_frame_label")) {
				addLabel("Frame: 0");
			}
		} else if (shouldPlay) {
			if (recorder.currentFrame() == 0) {
				shouldPlay = false;
				recorder.state = state::playing;
				PlayLayer::get()->resetLevel();
			}
		}

		if (recorder.state == state::off) {
			if (frameLabel != nullptr) {
				frameLabel->removeFromParent();
				frameLabel = nullptr;
			}
			if (stateLabel != nullptr) {
				stateLabel->removeFromParent();
				stateLabel = nullptr;
			}
		}
		
		if (recorder.state == state::recording) {
			if (stateLabel != nullptr) {
				if (stateLabel->getString() != "Recording" && Mod::get()->getSettingValue<bool>("show_recording_label"))
					stateLabel->setString("Recording");
				else if (!Mod::get()->getSettingValue<bool>("show_recording_label")) {
					stateLabel->removeFromParent();
					stateLabel = nullptr;
				}
			} else if (Mod::get()->getSettingValue<bool>("show_recording_label")) {
				addLabel("Recording");
			}
			if (Mod::get()->getSettingValue<bool>("frame_stepper") && stepFrame == false) 
				return;
			else if (stepFrame) {
				GJBaseGameLayer::update(1.f/fixedFps);
				stepFrame = false;
				recorder.syncMusic();
			} else GJBaseGameLayer::update(dt);
		} else GJBaseGameLayer::update(dt);
		
	}
};

void GJBaseGameLayerProcessCommands(GJBaseGameLayer* self) {
	if (recorder.state == state::recording) {
		if (((playerHolding && !Mod::get()->getSettingValue<bool>("vanilla")) ||
		Mod::get()->getSettingValue<bool>("frame_fix")) && !recorder.macro.empty()) {
			
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
			if (stateLabel != nullptr) {
				if (stateLabel->getString() != "Playing" && Mod::get()->getSettingValue<bool>("show_playing_label"))
					stateLabel->setString("Playing");
				else if (!Mod::get()->getSettingValue<bool>("show_playing_label")) {
					stateLabel->removeFromParent();
					stateLabel = nullptr;
				}
			} else if (Mod::get()->getSettingValue<bool>("show_playing_label")) {
				addLabel("Playing");
			}
			int frame = recorder.currentFrame();
        	while (recorder.currentAction < static_cast<int>(recorder.macro.size()) &&
			frame >= recorder.macro[recorder.currentAction].frame && !self->m_player1->m_isDead) {
            	auto& currentActionIndex = recorder.macro[recorder.currentAction];

				if (!safeModeEnabled) {
					safeModeEnabled = true;
					safeMode::updateSafeMode();
				}
				if (!Mod::get()->getSettingValue<bool>("override_macro_mode") && currentActionIndex.p1.xPos != 0) {
					if (self->m_player1->getPositionX() != currentActionIndex.p1.xPos ||
						self->m_player1->getPositionY() != currentActionIndex.p1.yPos)
							self->m_player1->setPosition(cocos2d::CCPoint(currentActionIndex.p1.xPos, currentActionIndex.p1.yPos));

						if (self->m_player1->m_isUpsideDown != currentActionIndex.p1.upsideDown && currentActionIndex.posOnly)
							self->m_player1->flipGravity(currentActionIndex.p1.upsideDown, true);

					
						if (currentActionIndex.p2.xPos != 0 && self->m_player2 != nullptr) {
							if (self->m_player2->getPositionX() != currentActionIndex.p2.xPos ||
							self->m_player2->getPositionY() != currentActionIndex.p2.yPos)
								self->m_player2->setPosition(cocos2d::CCPoint(currentActionIndex.p2.xPos, currentActionIndex.p2.yPos));

							if (self->m_player2->m_isUpsideDown != currentActionIndex.p2.upsideDown && currentActionIndex.posOnly)
								self->m_player2->flipGravity(currentActionIndex.p1.upsideDown, true);

						}
				} else {
				if ((currentActionIndex.p1.xPos != 0 && self->m_player1 != nullptr) && (!Mod::get()->getSettingValue<bool>("vanilla") || Mod::get()->getSettingValue<bool>("frame_fix"))) {
					if (((!Mod::get()->getSettingValue<bool>("vanilla") && !Mod::get()->getSettingValue<bool>("frame_fix")) && lastHold)
					|| Mod::get()->getSettingValue<bool>("frame_fix")) {
						if (self->m_player1->getPositionX() != currentActionIndex.p1.xPos ||
						self->m_player1->getPositionY() != currentActionIndex.p1.yPos)
							self->m_player1->setPosition(cocos2d::CCPoint(currentActionIndex.p1.xPos, currentActionIndex.p1.yPos));

						if (self->m_player1->m_isUpsideDown != currentActionIndex.p1.upsideDown && currentActionIndex.posOnly)
							self->m_player1->flipGravity(currentActionIndex.p1.upsideDown, true);

					
						if (currentActionIndex.p2.xPos != 0 && self->m_player2 != nullptr) {
							if (self->m_player2->getPositionX() != currentActionIndex.p2.xPos ||
							self->m_player2->getPositionY() != currentActionIndex.p2.yPos)
								self->m_player2->setPosition(cocos2d::CCPoint(currentActionIndex.p2.xPos, currentActionIndex.p2.yPos));

							if (self->m_player2->m_isUpsideDown != currentActionIndex.p2.upsideDown && currentActionIndex.posOnly)
								self->m_player2->flipGravity(currentActionIndex.p1.upsideDown, true);

						}
					}
				}
				}
				if (!currentActionIndex.posOnly) {
					self->handleButton(currentActionIndex.holding, currentActionIndex.button, currentActionIndex.player1);
					if (currentActionIndex.holding) lastHold = true;
					else lastHold = false;
				}

            	recorder.currentAction++;
        	}
			if (recorder.currentAction >= recorder.macro.size()) {
				if (stateLabel!=nullptr) stateLabel->removeFromParent();
				clearState(true);
			}
		}
	reinterpret_cast<void(__thiscall *)(GJBaseGameLayer *)>(base::get() + 0x1BD240)(self);
}

class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (recorder.state != state::off && restart != false) {
			leftOver = 0.f;
			restart = false;
		}
		
		playerHolding = false;

		if (safeModeEnabled) {
			safeModeEnabled = false;
			safeMode::updateSafeMode();
		}
		

		if (recorder.state == state::playing) {
			leftOver = 0.f;
			recorder.currentAction = 0;
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        	channel->setPitch(1);
		} else if (recorder.state == state::recording) {
        	if (this->m_isPracticeMode && !recorder.macro.empty() && recorder.currentFrame() != 0) {
  				int frame = recorder.currentFrame(); 
            	auto condition = [&](data& actionIndex) -> bool {
					return actionIndex.frame >= frame;
				};

            	recorder.macro.erase(remove_if(recorder.macro.begin(),
				recorder.macro.end(), condition),
				recorder.macro.end());

            	if (recorder.macro.back().holding && !recorder.macro.empty())
                	recorder.macro.push_back({
						recorder.macro.back().player1,
						frame,
						recorder.macro.back().button,
						false
					});

        	} else recorder.macro.clear();
   		}
	}

	void levelComplete() {
		PlayLayer::levelComplete();
		if (stateLabel!=nullptr) stateLabel->removeFromParent();
		if (recorder.state == state::recording)
			shouldPlay2 = true;
		
		clearState(true);
	}
};

class $modify(EndLevelLayer) {
	void onReplay(CCObject* s) {
		EndLevelLayer::onReplay(s);
		if (shouldPlay2 && Mod::get()->getSettingValue<bool>("auto_enable_play")) {
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
class $modify(CCScheduler) {
	void update(float dt) {
		if (recorder.state == state::off) return CCScheduler::update(dt);

		float speedhackValue = static_cast<float>(Mod::get()->getSettingValue<double>("speedhack"));

		if (recorder.state == state::recording) {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        	channel->setPitch(speedhackValue);
		} else {
			FMOD::ChannelGroup* channel;
        	FMODAudioEngine::sharedEngine()->m_system->getMasterChannelGroup(&channel);
        	channel->setPitch(1);
		}

		using namespace std::literals;
		float dt2 = (1.f / fixedFps);
		dt = (recorder.state == state::recording) ? dt * speedhackValue : dt;
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
	if (recorder.state == state::playing && leftOver > 1) {
		syncCooldown++;
		if (syncCooldown >= 20) {
			syncCooldown = 0;
			recorder.syncMusic();
		}
	}
	}
};

class $modify(CCKeyboardDispatcher) {
	bool dispatchKeyboardMSG(enumKeyCodes key, bool hold, bool p) {
		if (key == cocos2d::enumKeyCodes::KEY_C && hold && !p && recorder.state == state::recording) {
			if (!Mod::get()->getSettingValue<bool>("disable_speedhack")) {
				if (prevSpeed != 1 && Mod::get()->getSettingValue<double>("speedhack") == 1)
					Mod::get()->setSettingValue("speedhack", prevSpeed);
				else {
					prevSpeed = Mod::get()->getSettingValue<double>("speedhack");
					Mod::get()->setSavedValue<float>("previous_speed", prevSpeed);
					Mod::get()->setSettingValue("speedhack", 1.0);
				}
			}
		}

		if (key == cocos2d::enumKeyCodes::KEY_V && hold && !p && recorder.state == state::recording) {
			if (!Mod::get()->getSettingValue<bool>("disable_frame_stepper")) {
				if (Mod::get()->getSettingValue<bool>("frame_stepper")) stepFrame = true;
				else Mod::get()->setSettingValue("frame_stepper", true);
			}
		}

		if (key == cocos2d::enumKeyCodes::KEY_B && hold && !p && recorder.state == state::recording) {
			if (Mod::get()->getSettingValue<bool>("frame_stepper")) {
				recorder.syncMusic();
				Mod::get()->setSettingValue("frame_stepper", false);
			}
		}
		return CCKeyboardDispatcher::dispatchKeyboardMSG(key,hold,p);
	}
};

$execute {
	if (Mod::get()->getSavedValue<float>("previous_speed"))
		prevSpeed = Mod::get()->getSavedValue<float>("previous_speed");
	
	Mod::get()->hook(reinterpret_cast<void *>(base::get() + 0x1BD240), &GJBaseGameLayerProcessCommands, "GJBaseGameLayer::processCommands", tulip::hook::TulipConvention::Thiscall);
	for (std::size_t i = 0; i < 15; i++) {
		safeMode::patches[i] = Mod::get()->patch(reinterpret_cast<void*>(base::get() + std::get<0>(safeMode::codes[i])),
		std::get<1>(safeMode::codes[i])).unwrap();
		safeMode::patches[i]->disable();
	}
}
