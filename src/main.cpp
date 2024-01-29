#include "fileSystem.hpp"
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cocos2d.h>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/binding/GameManager.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <vector>
#include <nfd.h>
#include <chrono>

float leftOver = 0.f; // For CCScheduler
int fixedFps = 240;
bool restart = false;
const int playerEnums[2][3] = {
    {cocos2d::enumKeyCodes::KEY_ArrowUp, cocos2d::enumKeyCodes::KEY_ArrowLeft, cocos2d::enumKeyCodes::KEY_ArrowRight}, 
    {cocos2d::enumKeyCodes::KEY_W, cocos2d::enumKeyCodes::KEY_A, cocos2d::enumKeyCodes::KEY_D}
};

void releaseKeys() {
	for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 3; ++col) {
			cocos2d::CCKeyboardDispatcher::get()->dispatchKeyboardMSG(static_cast<cocos2d::enumKeyCodes>(playerEnums[row][col]), false, false);
        }
    }
}

using namespace geode::prelude;

struct data {
    bool player1;
    int frame;
    int button;
    bool holding;
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
	void recordAction(bool holding, int button, bool player1, int frame, GJBaseGameLayer* bgl) {
		bool p1 = (GameManager::get()->getGameVariable("0010") && !bgl->m_levelSettings->m_platformerMode) ? !player1 : player1;
    	macro.push_back({p1,frame,button,holding});
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

		this->setTitle("xdBot");
		auto menu = CCMenu::create();
    	menu->setPosition({0, 0});
    	m_mainLayer->addChild(menu);

 		auto checkOffSprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
   		auto checkOnSprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

		CCPoint corner = winSize/2.f-CCPOINT_CREATE(m_size.width/2.f,-m_size.height/2.f);
 
		auto label = CCLabelBMFont::create("Record", "bigFont.fnt"); 
    	label->setAnchorPoint({0, 0.5});
    	label->setScale(0.7f);
    	label->setPosition(corner + CCPOINT_CREATE(168, -60));
    	m_mainLayer->addChild(label);

    	recording = CCMenuItemToggler::create(checkOffSprite,
		checkOnSprite,
		this,
		menu_selector(RecordLayer::toggleRecord));

    	recording->setPosition(label->getPosition() + CCPOINT_CREATE(105,0));
    	recording->setScale(0.85f);
    	recording->toggle(recorder.state == state::recording); 
    	menu->addChild(recording);

    	label = CCLabelBMFont::create("Play", "bigFont.fnt");
    	label->setScale(0.7f);
    	label->setPosition(corner + CCPOINT_CREATE(198, -90)); 
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

   		auto btn = CCMenuItemSpriteExtra::create(btnSprite,
   		this,
   		menu_selector(saveMacroPopup::openSaveMacro));

    	btn->setPosition(corner + CCPOINT_CREATE(65, -160)); 
    	menu->addChild(btn);

		btnSprite = ButtonSprite::create("Load");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(loadMacroPopup::openLoadMenu));

    	btn->setPosition(corner + CCPOINT_CREATE(144, -160));
    	menu->addChild(btn);

  		btnSprite = ButtonSprite::create("Clear");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(RecordLayer::clearMacro));

    	btn->setPosition(corner + CCPOINT_CREATE(228, -160));
    	menu->addChild(btn);

		infoMacro = CCLabelBMFont::create("", "chatFont.fnt");
    	infoMacro->setAnchorPoint({0, 1});
    	infoMacro->setPosition(corner + CCPOINT_CREATE(21, -57));
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
 
	void updateInfo() {
 		std::stringstream infoText;
    	infoText << "Current Macro:";
    	infoText << "\nSize: " << recorder.macro.size();
		infoText << "\nDuration: " << (!recorder.macro.empty() 
		? recorder.macro.back().frame / fixedFps : 0) << "s";
    	infoMacro->setString(infoText.str().c_str());
	}

	void togglePlay(CCObject*) {
		if (recorder.state == state::recording) recording->toggle(false);
    	recorder.state = (recorder.state == state::playing) ? state::off : state::playing;

		if (recorder.state == state::playing) restart = true;
		else if (recorder.state == state::off) restart = false;
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
			} else if (recorder.state == state::off) restart = false;
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
	create()->show();
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
 	std::ofstream file(savePath);
	if (file.is_open()) {
		for (auto &action : recorder.macro) {
			file << action.frame << "|" << action.holding <<
			"|" << action.button << "|" << action.player1 << "\n";
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
    std::ifstream file(loadPath);
	std::string line;
	if (!file.is_open()) {
		FLAlertLayer::create(
    	"Load Macro",   
    	"An <cr>error</c> occurred while loading this macro.",  
    	"OK"      
		)->show();
		return;
	}
	while (std::getline(file, line)) {
		std::istringstream isSS(line);
		int holding;
		int frame;
		int button;
		int player1;
		char separator;
		if (isSS >> frame >> separator >> holding >> separator >> button >> 
		separator >> player1 && separator == '|') {
			recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding});
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

void clearState() {
	if (recorder.state != state::off) {
		recorder.state = state::off;
		leftOver = 0.f;
	}
}

class $modify(PauseLayer) {
	void customSetup() {
		auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition(winSize.width-36, winSize.height - 70.f);
        this->addChild(menu);
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_stopEditorBtn_001.png");
        sprite->setScale(0.75f);

        auto btn = CCMenuItemSpriteExtra::create(sprite,
		this,
		menu_selector(RecordLayer::openMenu));

        menu->addChild(btn);
		PauseLayer::customSetup();
	} 

	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		clearState();
	}

	void goEdit() {
		PauseLayer::goEdit();
		clearState();
	}

	void onResume(CCObject* sender) {
		PauseLayer::onResume(sender);
		if (restart) PlayLayer::get()->resetLevel();
	}

	void onPracticeMode(CCObject* sender) {
		PauseLayer::onPracticeMode(sender);
		if (restart) PlayLayer::get()->resetLevel();
	}

};

class $modify(GJBaseGameLayer) {
	void handleButton(bool holding, int button, bool player1) {
		GJBaseGameLayer::handleButton(holding,button,player1);
		if (recorder.state == state::recording) {
			int frame = recorder.currentFrame(); 
			recorder.recordAction(holding, button, player1, frame, this);
		}
	}

	int getPlayer1(int p1, GJBaseGameLayer* bgl) {
		bool player1;
		if (GameManager::get()->getGameVariable("0010") && !bgl->m_levelSettings->m_platformerMode) player1 = !p1;
		else player1 = p1;
		return static_cast<int>(player1);
	}

	void update(float dt) {
		GJBaseGameLayer::update(dt);
		if (recorder.state == state::playing) {
			int frame = recorder.currentFrame();
        	while (recorder.currentAction < static_cast<int>(recorder.macro.size()) &&
			frame >= recorder.macro[recorder.currentAction].frame && !this->m_player1->m_isDead) {
            	auto& currentActionIndex = recorder.macro[recorder.currentAction];
        		cocos2d::CCKeyboardDispatcher::get()->dispatchKeyboardMSG(
				static_cast<cocos2d::enumKeyCodes>(playerEnums[getPlayer1(currentActionIndex.player1, this)][currentActionIndex.button-1]),
				currentActionIndex.holding, false);
            	recorder.currentAction++;
        	}
			if (recorder.currentAction >= recorder.macro.size()) recorder.state = state::off;
    	} else if (recorder.state == state::recording) {
			
		}
	}
};

	// ---------------- Hooks ---------------- //

class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (recorder.state != state::off && restart != false) restart = false;
		if (recorder.state == state::playing) {
			recorder.currentAction = 0;
			leftOver = 0.f;
			releaseKeys();
		} else if (recorder.state == state::recording) {
        	if (this->m_isPracticeMode && !recorder.macro.empty()) {
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
		clearState();
	}
};

class $modify(CCScheduler) {
	void update(float dt) {
		if (recorder.state == state::off) return CCScheduler::update(dt);
		using namespace std::literals;
		float dt2 = 1.f / fixedFps;
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
	}
};
