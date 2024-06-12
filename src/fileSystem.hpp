#include <Geode/Geode.hpp>
#include <locale>
#include <codecvt>
#include <string>

#ifdef GEODE_IS_ANDROID
    std::string slash = "/";
    bool isAndroid = true;
#else
    std::string slash = "\\";
    bool isAndroid = false;
#endif

bool refreshMenu = false;
std::string searchString = "";

std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

#define CCPOINT_CREATE(__X__, __Y__) cocos2d::CCPointMake((float)(__X__), (float)(__Y__))

using namespace geode::prelude;

class macroCell : public CCNode {
public:
    static macroCell* create(std::string name) {
        auto ret = new macroCell();
        if (ret && ret->init(name)) {
            ret->autorelease();
        } else {
            delete ret;
            ret = nullptr;
        }
        return ret;
    }

    void loadMacro(CCObject*);
    void handleLoad(CCObject*);

    void handleDelete(CCObject* btn) {
        std::string path = Mod::get()->getSaveDir().string() + slash + static_cast<CCMenuItemSpriteExtra*>(btn)->getID() + ".xd";

        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wideString = converter.from_bytes(path);
        std::locale utf8_locale(std::locale(), new std::codecvt_utf8<wchar_t>);

        try {
            std::filesystem::remove(wideString);
        } catch (const std::filesystem::filesystem_error& e) {
            FLAlertLayer::create(
                "Delete Macro",
                "There was an <cr>error</c> deleting this macro.",
                "OK"
            )->show();
            return;
        }

        if (this->getParent()->getParent()->getChildren()->count() == 1) {
            CCLabelBMFont* noMacroLbl = CCLabelBMFont::create("No macros.", "bigFont.fnt");
            noMacroLbl->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
            noMacroLbl->setColor(ccc3(201, 170, 153));
            noMacroLbl->setScale(0.725f);
            this->getParent()->getParent()->getParent()->addChild(noMacroLbl);
            noMacroLbl->setPosition(this->getParent()->getParent()->getParent()->getContentSize() / 2);
        }

        this->getParent()->removeFromParentAndCleanup(true);

        FLAlertLayer::create(
            "Delete Macro",
            "Macro deleted <cg>successfully</c>.",
            "OK"
        )->show();
    }

    void deleteMacro(CCObject* btn) {
        geode::createQuickPopup(
            "Delete Macro",
            "<cr>Delete</c> this macro?",
            "Cancel", "Ok",
            [this, btn](auto, bool btn2) {
                if (btn2) this->handleDelete(btn);
            }
        );
    }

    bool init(std::string name) {
        auto lbl = (name.length() > 19) ? (name.substr(0, 19) + "...") : name;
        CCLabelBMFont* macroLabel = CCLabelBMFont::create(lbl.c_str(), "bigFont.fnt");
        macroLabel->setAnchorPoint(ccp(0.0f, 0.5f));
        macroLabel->setAlignment(CCTextAlignment::kCCTextAlignmentLeft);
        macroLabel->setPosition({5, 20});
        macroLabel->setScale(0.45f);
        this->addChild(macroLabel);
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});

        auto btnSpr1 = ButtonSprite::create("Load");
        btnSpr1->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(btnSpr1, this, menu_selector(macroCell::loadMacro));
        btn->setPosition({270, 20});
        menu->addChild(btn);
        btn->setID(name);

        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        btnSpr->setScale(0.65f);
        btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(macroCell::deleteMacro));
        btn->setPosition({225, 20});
        btn->setID(name);
        menu->addChild(btn);

        this->addChild(menu);
        return true;
    }
};

class searchMacroPopup : public geode::Popup<std::string const&> {
public:
    CCTextInputNode* macroNameInput = nullptr;

    bool setup(std::string const& value) override {
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto inputBg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
        inputBg->setScale(0.7f);
        inputBg->setColor({0, 0, 0});
        inputBg->setOpacity(75);
        inputBg->setPosition(winSize / 2 + CCPOINT_CREATE(0, 7));
        inputBg->setContentSize({235, 54});

        macroNameInput = CCTextInputNode::create(150, 30, "Search", "bigFont.fnt");
        macroNameInput->m_textField->setAnchorPoint({0.5f, 0.5f});
        macroNameInput->ignoreAnchorPointForPosition(true);
        macroNameInput->m_placeholderLabel->setAnchorPoint({0.5f, 0.5f});
        macroNameInput->setPosition(winSize / 2 + CCPOINT_CREATE(0, 7));
        macroNameInput->setMaxLabelScale(0.7f);
        macroNameInput->setLabelPlaceholderColor(ccc3(163, 135, 121));
        macroNameInput->setMouseEnabled(true);
        macroNameInput->setTouchEnabled(true);

        auto title = CCLabelBMFont::create("Search Macro", "bigFont.fnt");
        title->setPosition(winSize / 2 + CCPOINT_CREATE(0, 50));
        title->setScale(0.6f);
        m_mainLayer->addChild(title);

        auto btnSpr = ButtonSprite::create("Search");
        btnSpr->setScale(0.9f);
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(searchMacroPopup::searchMacro));
        btn->setPosition(winSize / 2 + CCPOINT_CREATE(0, -40));
        menu->addChild(btn);
        m_mainLayer->addChild(menu);

        m_mainLayer->addChild(inputBg);
        m_mainLayer->addChild(macroNameInput);
        return true;
    }

    static searchMacroPopup* create() {
        auto ret = new searchMacroPopup();
        if (ret && ret->init(220, 140, "")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void clearSearch(CCObject*) {
        if (searchString == "") return;
        searchString = "";
        refreshMenu = true;
    }

    void openSearchMacro(CCObject*);
    void searchMacro(CCObject*);
};

class loadMacroPopup : public geode::Popup<std::string const&> {
public:
    void importMacro(CCObject*) {
        file::FilePickOptions fileOptions;
        file::FilePickOptions::Filter textFilter;
        textFilter.description = "Macro Files";
        textFilter.files = {"*.xd"};
        fileOptions.filters.push_back(textFilter);

        file::pickFile(file::PickMode::OpenFile, fileOptions, [this](ghc::filesystem::path result) {
            auto path = ghc::filesystem::path(result.c_str());

            std::ifstream sourceMacro(path);
            if (!sourceMacro.is_open()) {
                FLAlertLayer::create(
                    "Import Macro",
                    "An <cr>error</c> occurred while importing this macro.",
                    "OK"
                )->show();
                return;
            }

            std::ofstream copiedMacro(Mod::get()->getSaveDir().string() + slash + path.filename().string(), std::ios::binary);
            if (!copiedMacro.is_open()) {
                FLAlertLayer::create(
                    "Import Macro",
                    "An <cr>error</c> occurred while importing this macro.",
                    "OK"
                )->show();
                return;
            }

            copiedMacro << sourceMacro.rdbuf();
            sourceMacro.close();
            copiedMacro.close();
            refresh();
            FLAlertLayer::create(
                "Import Macro",
                "Macro imported <cg>successfully</c>.",
                "OK"
            )->show();
        });
    }

    bool setup(std::string const& value) override {
        CCArray* macroList = CCArray::create();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        Result<std::vector<ghc::filesystem::path>> macros;
        std::string titleLol;
        std::string noBUMessage;
        CCPoint corner = winSize / 2.f - CCPOINT_CREATE(m_size.width / 2.f, m_size.height / 2.f);
        macros = file::readDirectory(Mod::get()->getSaveDir());
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        if (!macros) titleLol = "An error occurred.";
        else if (macros && !macros.value().empty()) titleLol = "Load Macro";
        else titleLol = "No Macros";
        auto title = CCLabelBMFont::create(titleLol.c_str(), "bigFont.fnt");
        title->setPosition(corner.x + m_size.width / 2, corner.y + m_size.height - 25);
        title->setScale(0.6f);
        m_mainLayer->addChild(title);
        auto backBtnSpr = ButtonSprite::create("Search");
        backBtnSpr->setScale(0.9f);
        auto backBtn = CCMenuItemSpriteExtra::create(backBtnSpr, this, menu_selector(loadMacroPopup::openSearchMacro));
        backBtn->setPosition(corner.x + 25, corner.y + m_size.height - 25);
        menu->addChild(backBtn);
        auto importBtnSpr = ButtonSprite::create("Import");
        importBtnSpr->setScale(0.9f);
        auto importBtn = CCMenuItemSpriteExtra::create(importBtnSpr, this, menu_selector(loadMacroPopup::importMacro));
        importBtn->setPosition(corner.x + m_size.width - 25, corner.y + m_size.height - 25);
        menu->addChild(importBtn);

        if (macros) {
            for (auto& path : macros.value()) {
                if (path.extension().string() == ".xd") {
                    macroList->addObject(CCString::create(path.filename().string()));
                }
            }
        }

        CCLabelBMFont* noMacroLbl = CCLabelBMFont::create("No macros.", "bigFont.fnt");
        noMacroLbl->setAlignment(CCTextAlignment::kCCTextAlignmentCenter);
        noMacroLbl->setColor(ccc3(201, 170, 153));
        noMacroLbl->setScale(0.725f);
        if (macroList->count() == 0) {
            noMacroLbl->setPosition(winSize / 2);
            m_mainLayer->addChild(noMacroLbl);
        } else {
            auto m_pButtonMenu = CCMenu::create();
            m_pButtonMenu->setPosition({0, 0});
            m_mainLayer->addChild(m_pButtonMenu);
            auto bgSprite = extension::CCScale9Sprite::create("square02b_001.png");
            bgSprite->setContentSize({325.f, 210.f});
            bgSprite->setColor({0, 0, 0});
            bgSprite->setOpacity(75);
            auto bgNode = CCNode::create();
            m_mainLayer->addChild(bgNode, -1);
            bgNode->setPosition(winSize / 2);
            bgNode->addChild(bgSprite);
            for (int i = 0; i < macroList->count(); i++) {
                std::string currentMacro = static_cast<CCString*>(macroList->objectAtIndex(i))->getCString();
                if (!toLower(currentMacro).contains(toLower(searchString)) && searchString != "") continue;
                m_pButtonMenu->addChild(
                    macroCell::create(currentMacro),
                    i,
                    i + 1
                );
                m_pButtonMenu->getChildByTag(i + 1)->setPosition(0, 90 - i * 45);
                bgSprite->setContentSize({325.f, std::max(210.f, static_cast<float>(macroList->count() * 45))});
            }
            bgNode->setPosition(
                winSize.width / 2,
                winSize.height / 2 - std::max(0.f, static_cast<float>((macroList->count() - 4) * 25))
            );
        }
        refreshMenu = false;
        m_mainLayer->addChild(menu);
        return true;
    }

    static loadMacroPopup* create() {
        auto ret = new loadMacroPopup();
        if (ret && ret->init(350.f, 270.f, "")) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
    void openSearchMacro(CCObject*) {
        auto popup = searchMacroPopup::create();
        popup->show();
    }
    void refresh() {
        auto popup = loadMacroPopup::create();
        popup->show();
    }
    void searchMacro(CCObject*);
};

class macroLayer : public CCLayer {
public:
    bool init() {
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        auto btnSpr = ButtonSprite::create("Load Macro");
        btnSpr->setScale(0.8f);
        auto btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(macroLayer::loadMacro));
        btn->setPosition({50, 30});
        menu->addChild(btn);
        this->addChild(menu);
        return true;
    }
    static macroLayer* create() {
        auto ret = new macroLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
    void loadMacro(CCObject*) {
        auto popup = loadMacroPopup::create();
        popup->show();
    }
};

class $modify(m) MenuLayer : MenuLayer {
    void onMoreGames(CCObject* btn) {
        auto newLayer = macroLayer::create();
        CCDirector::sharedDirector()->getRunningScene()->addChild(newLayer, 100);
    }
};
