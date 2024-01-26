#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <vector>
#include <nfd.h>
#include <chrono>

#define CCPOINT_CREATE(__X__,__Y__) cocos2d::CCPointMake((float)(__X__), (float)(__Y__))

int fixedFps = 240;
bool restart = false;

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
	void record_action(bool holding, int button, bool player1, int frame) {
    	macro.push_back({player1,frame,button,holding});
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
    	this->addChild(menu);

 		auto checkOffSprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
   		auto checkOnSprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

		CCPoint corner = winSize/2.f-CCPOINT_CREATE(m_size.width/2.f,-m_size.height/2.f);
 
		auto label = CCLabelBMFont::create("Record", "bigFont.fnt"); 
    	label->setAnchorPoint({0, 0.5});
    	label->setScale(0.7f);
    	label->setPosition(corner + CCPOINT_CREATE(168, -60));
    	this->addChild(label);

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
    	label->setPosition(corner + CCPOINT_CREATE(198, -105)); 
    	label->setAnchorPoint({0, 0.5});
    	this->addChild(label);

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
   		menu_selector(RecordLayer::saveMacro));

    	btn->setPosition(corner + CCPOINT_CREATE(65, -160)); 
    	menu->addChild(btn);

		btnSprite = ButtonSprite::create("Load");
		btnSprite->setScale(0.72f);

    	btn = CCMenuItemSpriteExtra::create(btnSprite,
		this,
		menu_selector(RecordLayer::loadMacro));

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
    	this->addChild(infoMacro);

        return true;
    }

	void updateInfo() {
 		std::stringstream infoText;
    	infoText << "Current Macro:";
    	infoText << "\nSize: " << recorder.macro.size();
		infoText << "\nDuration: " << (!recorder.macro.empty() 
		? recorder.macro.back().frame / fixedFps : 0) << "s";
    	infoMacro->setString(infoText.str().c_str());
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
 	void handleLoad() {
		nfdchar_t* loadPath = nullptr;
    	auto load = NFD_OpenDialog("xd", nullptr, &loadPath);
    	if (load != NFD_OKAY) return;
		recorder.macro.clear();
        std::ifstream file(loadPath);
		std::string line;
		if (!file.is_open()) {
			FLAlertLayer::create(
    		"Load Error",   
    		"An error occurred while loading this macro.",  
    		"OK"      
			)->show();
			return;
		}
		while (std::getline(file, line)) {
			std::istringstream isSS(line);
			int holding, frame, button, player1;
			char separator;
			if (isSS >> holding >> separator >> frame >> separator >> button >> 
			separator >> player1 && separator == '|') {
				recorder.macro.push_back({(bool)player1, (int)frame, (int)button, (bool)holding});
				updateInfo();
			}
		}				
		file.close();
		free(loadPath);
   }
	void loadMacro(CCObject*) {
		if (!recorder.macro.empty()) {
			geode::createQuickPopup(
    		"Load Macro",     
    		"<cr>Overwrite</c> the current macro?", 
    		"Cancel", "Ok",  
    		[this](auto, bool btn2) {
        		if (btn2) this->handleLoad();
    		});
		} else handleLoad();
	}
	void saveMacro(CCObject*) {
		if (recorder.macro.empty()) {
			FLAlertLayer::create(
    		"Save Macro",   
    		"You can't save an <cl>empty</c> macro.",  
    		"OK"      
			)->show();
			return;
		}
		nfdchar_t* savePath = nullptr;
		auto save = NFD_SaveDialog("xd", nullptr, &savePath);
		if (save == NFD_OKAY) {
 			std::ofstream file(savePath);
			if (file.is_open()) {
				for (auto &action : recorder.macro) {
					file << action.frame << "|" << action.holding <<
					"|" << action.button << "|" << action.player1 << "\n";
				}
				file.close();
			}
		}
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
			}
    	});
	}

    void openMenu(CCObject*) {
		auto layer = create();
		layer->m_noElasticity = true;
		layer->show();
	}
};


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
		if (recorder.state != state::off) recorder.state = state::off;
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
			recorder.record_action(holding, button, player1, frame);
		} 
	}

	void update(float dt) {
		GJBaseGameLayer::update(dt);
		if (recorder.state == state::playing) {
			int frame = recorder.currentFrame();
        	while (recorder.currentAction < static_cast<int>(recorder.macro.size()) &&
			frame >= recorder.macro[recorder.currentAction].frame && !this->m_player1->m_isDead) {
            	auto& currentActionIndex = recorder.macro[recorder.currentAction];
        		GJBaseGameLayer::handleButton(currentActionIndex.holding, currentActionIndex.button, currentActionIndex.player1);
            	recorder.currentAction++;	
        	}
			if (recorder.currentAction >= recorder.macro.size()) recorder.state = state::off;
    	}
	}
};

class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (recorder.state != state::off && restart != false) restart = false;
		if (recorder.state == state::playing) recorder.currentAction = 0;
		else if (recorder.state == state::recording) {
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
		if (recorder.state != state::off) recorder.state = state::off;
	}
};

float leftOver = 0.f;
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
