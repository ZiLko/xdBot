#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <vector>
#include <nfd.h>
#include <chrono>

int fixedFps = 240;

using namespace geode::prelude;
struct data
{
    bool player1;
    int frame;
    int button;
    bool holding;
};
int currentFrame() {
	return static_cast<int>((*(double*)(((char*)PlayLayer::get()) + 0x328)) * fixedFps); //m_time * fps
}
enum state {
    off,
    recording,
    playing};

class recordSystem {
    public:
    state state = off;

 size_t currentAction = 0;
   std::vector<data> macro;
void record_action(bool holding, int button, bool player1, int frame) {
    macro.push_back({player1,frame,button,holding});
}

};

PauseLayer* pLayer;
recordSystem recorder;
bool restart = false;

class RecordLayer : public geode::Popup<std::string const&> {
 CCLabelBMFont* info_macro;
 CCMenuItemToggler* recording;
    CCMenuItemToggler* playing;
    protected:
    bool setup(std::string const& value) override {
        auto win_size = cocos2d::CCDirector::sharedDirector()->getWinSize();
   auto m_pLayer = this;
this->setTitle("xdBot");
auto menu = CCMenu::create();
    menu->setPosition({0, win_size.height});

    m_pLayer->addChild(menu);
 auto* const check_off_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto* const check_on_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

auto label = CCLabelBMFont::create("Record", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.7f);
    label->setPosition({300, win_size.height-120});
    m_pLayer->addChild(label);

    recording = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(RecordLayer::toggle_record));
    recording->setPosition({405, -120});
    recording->setScale(0.85f);
    recording->toggle(recorder.state == state::recording); 
    menu->addChild(recording);

    label = CCLabelBMFont::create("Play", "bigFont.fnt");
    label->setScale(0.7f);
    label->setPosition({330, win_size.height-155});
    label->setAnchorPoint({0, 0.5});
    m_pLayer->addChild(label);
     playing = CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(RecordLayer::toggle_play));
    playing->setPosition({405, -155});
    playing->setScale(0.85f);
    playing->toggle(recorder.state == state::playing); 
    menu->addChild(playing);


 auto btnSprite = ButtonSprite::create("Save");
    btnSprite->setScale(0.72f);

   auto btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(RecordLayer::save_macro));
    btn->setPosition({200, -210});
    menu->addChild(btn);

btnSprite = ButtonSprite::create("Load");
btnSprite->setScale(0.72f);
    btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(RecordLayer::load_macro));
    btn->setPosition({278.5f, -210});
    menu->addChild(btn);

  btnSprite = ButtonSprite::create("Clear");
btnSprite->setScale(0.72f);
    btn = CCMenuItemSpriteExtra::create(btnSprite, this, menu_selector(RecordLayer::clear_macro));
    btn->setPosition({363, -210});
    menu->addChild(btn);

info_macro = CCLabelBMFont::create("", "chatFont.fnt");
    info_macro->setAnchorPoint({0, 1});
    info_macro->setPosition({160, win_size.height - 112});
	updateInfo();
    m_pLayer->addChild(info_macro);
        return true;
    }

void updateInfo() {
 std::stringstream infoString;
    infoString << "Current Macro:";
    infoString << "\nSize: " << recorder.macro.size();
	infoString << "\nDuration: " << (!recorder.macro.empty() ? recorder.macro.back().frame / fixedFps : 0) << "s";
    info_macro->setString(infoString.str().c_str());
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
    if (load == NFD_OKAY) {
		recorder.macro.clear();
        std::ifstream file(loadPath);
		std::string line;
		if (file.is_open()) {
		while (std::getline(file, line)) {
		std::istringstream isSS(line);
		int value1, value2, value3, value4;
		char separator;
		if (isSS >> value1 >> separator >> value2 >> separator >> value3 >> separator >> value4 && separator == '|') {
		recorder.macro.push_back({(bool)value4, (int)value1, (int)value3, (bool)value2});
		updateInfo();
}
}
file.close();
} else {
FLAlertLayer::create(
    "Load Error",   
    "An error occurred while loading this macro.",  
    "OK"      
)->show();
}
free(loadPath);
    }
   }
void load_macro(CCObject*) {
if (!recorder.macro.empty()) {
geode::createQuickPopup(
    "Load Macro",     
    "<cr>Overwrite</c> the current macro?", 
    "Cancel", "Ok",  
    [this](auto, bool btn2) {
        if (btn2) {this->handleLoad();}
    }
);
} else {handleLoad();}
}
void save_macro(CCObject*) {
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
file << action.frame << "|" << action.holding << "|" << action.button << "|" << action.player1 << "\n";
}
file.close();
}
}
}

void toggle_play(CCObject*) {
if (recorder.state == state::recording) {recording->toggle(false);}
    recorder.state = (recorder.state == state::playing) ? state::off : state::playing;
	if (recorder.state == state::playing) {restart = true;} else if (recorder.state == state::off) {
		restart = false;}
}
void toggle_record(CCObject* w) {
	if(!recorder.macro.empty() && recorder.state != state::recording) {geode::createQuickPopup(
    "Warning",     
    "This will <cr>clear</c> the current macro.", 
    "Cancel", "Ok",  
    [this, w](auto, bool btn2) {
        if (!btn2) {
			this->recording->toggle(false);
		} else {
		recorder.macro.clear();
		this->toggle_record(w);
		}
    }
);
} else {
	if (recorder.state == state::playing) {this->playing->toggle(false);}
    recorder.state = (recorder.state == state::recording) ? state::off : state::recording;
	if (recorder.state == state::recording) {
		restart = true;
		updateInfo();
	} else if (recorder.state == state::off) {
		restart = false;}}
}

void clear_macro(CCObject*) {
	if (recorder.macro.empty()) {return;}
	geode::createQuickPopup(
    "Clear Macro",     
    "<cr>Clear</c> the current macro?", 
    "Cancel", "Yes",  
    [this](auto, bool btn2) {
        if (btn2) {recorder.macro.clear();
	this->updateInfo();}
    }
);
   
}
    void openMenu(CCObject*) {
		auto layer = create();
layer->m_noElasticity = true;
layer->show();
	}
};


class $modify(PauseLayer) {
	void customSetup() {
        pLayer = this;
auto win_size = CCDirector::sharedDirector()->getWinSize();
        auto menu = CCMenu::create();
        menu->setPosition(win_size.width-36, win_size.height - 70.f);
        this->addChild(menu);
        auto sprite = CCSprite::createWithSpriteFrameName("GJ_stopEditorBtn_001.png");
        sprite->setScale(0.75f);
        auto btn = CCMenuItemSpriteExtra::create(sprite, this, menu_selector(RecordLayer::openMenu));
        menu->addChild(btn);
		PauseLayer::customSetup();
	} 

	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		if (recorder.state != state::off) {recorder.state = state::off;}
	}

	void onResume(CCObject* sender) {
		PauseLayer::onResume(sender);
		if (restart) {PlayLayer::get()->resetLevel();}
	}

	void onPracticeMode(CCObject* sender) {
		PauseLayer::onPracticeMode(sender);
		if (restart) {PlayLayer::get()->resetLevel();}
	}

};
class $modify(GJBaseGameLayer) {
	void handleButton(bool holding, int button, bool player1) {
GJBaseGameLayer::handleButton(holding,button,player1);
if (recorder.state == state::recording) {
	int frame = currentFrame(); 
	recorder.record_action(holding, button, player1, frame);} 
	}

	void update(float dt) {
		GJBaseGameLayer::update(dt);
if (recorder.state == state::playing) {
	int frame = currentFrame();
        while (recorder.currentAction < static_cast<int>(recorder.macro.size()) && frame >= recorder.macro[recorder.currentAction].frame) {
            auto& currentActionIndex = recorder.macro[recorder.currentAction];
           PlayLayer::get()->handleButton(currentActionIndex.holding, currentActionIndex.button, currentActionIndex.player1);
            recorder.currentAction++;	
        }
		if (recorder.currentAction >= recorder.macro.size()) {recorder.state = state::off;}
    } 
	}

};

class $modify(PlayLayer) {
	void resetLevel() {
		PlayLayer::resetLevel();
		if (recorder.state != state::off && restart != false) {restart = false;}
if (recorder.state == state::playing) {recorder.currentAction = 0;
} else if (recorder.state == state::recording) {
        if (this->m_isPracticeMode && !recorder.macro.empty()) {
  int frame = currentFrame(); 
            auto condition = [&](data& actionIndex) -> bool {return actionIndex.frame >= frame;};

            recorder.macro.erase(remove_if(recorder.macro.begin(), recorder.macro.end(), condition), recorder.macro.end());

            if (recorder.macro.back().holding && !recorder.macro.empty()) {
                recorder.macro.push_back({recorder.macro.back().player1, frame, recorder.macro.back().button, false});
            }
        }
        else {
            recorder.macro.clear();
        }
    } 
	}
	void levelComplete() {
		PlayLayer::levelComplete();
		if (recorder.state != state::off) {recorder.state = state::off;}
	}

};
float leftOver = 0.f;
class $modify(CCScheduler) {
void update(float dt) {
	if (recorder.state != state::playing) {return CCScheduler::update(dt);}
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
