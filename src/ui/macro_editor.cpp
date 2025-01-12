#include "macro_editor.hpp"
#include "record_layer.hpp"

#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/FLAlertLayer.hpp>

MacroEditLayer* editLayer = nullptr;

#ifdef GEODE_IS_WINDOWS

class $modify(CCEGLView) {
    void onGLFWMouseMoveCallBack(GLFWwindow* v1, double v2, double v3) {
        CCEGLView::onGLFWMouseMoveCallBack(v1, v2, v3);

        if (!editLayer) return;

        CCScene* scene = CCDirector::get()->getRunningScene();
        if (MacroEditLayer* layer = scene->getChildByType<MacroEditLayer>(0))
            editLayer = layer;
        else
            return;

        editLayer->updateHover(getMousePos());
        
    }
};

#endif

class $modify(FLAlertLayer) {

#ifdef GEODE_IS_ANDROID

    virtual bool ccTouchBegan(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        if (!FLAlertLayer::ccTouchBegan(touch, event)) return false;
        MacroEditLayer* layer = typeinfo_cast<MacroEditLayer*>(this);

        if (!layer) return true;

        layer->updateHover(touch->getLocation());

        return true;
    }

    virtual void ccTouchMoved(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        FLAlertLayer::ccTouchMoved(touch, event);
        MacroEditLayer* layer = typeinfo_cast<MacroEditLayer*>(this);

        if (!layer) return;

        layer->updateHover(touch->getLocation());
    }

#endif

    virtual void ccTouchEnded(cocos2d::CCTouch * touch, cocos2d::CCEvent * event) {
        FLAlertLayer::ccTouchEnded(touch, event);
        MacroEditLayer* layer = typeinfo_cast<MacroEditLayer*>(this);

        if (!layer) return;

        layer->selectInput(layer->hoveredInput);
        layer->updateHover(touch->getLocation());
    }

};

MacroEditLayer::~MacroEditLayer() {
    editLayer = nullptr;
};

void MacroEditLayer::onClose(CCObject*) {
    if (!saved) {
        geode::createQuickPopup(
            "Exit",
            "<cr>Exit</c> without applying changes?",
            "Cancel", "Yes",
            [this](auto, bool btn2) {
                if (!btn2) return;
                editLayer = nullptr;
                this->setKeypadEnabled(false);
                this->setTouchEnabled(false);
                this->removeFromParentAndCleanup(true);
            }
        );
    } else {
        editLayer = nullptr;
        this->setKeypadEnabled(false);
        this->setTouchEnabled(false);
        this->removeFromParentAndCleanup(true);
    }
}

void MacroEditLayer::updateHover(cocos2d::CCPoint pos) {
    if (pageInputs.empty()) return;

    cocos2d::CCSize center = CCDirector::sharedDirector()->getWinSize() / 2.f;

    float xMin = center.width - 91 - 95;
    float xMax = center.width - 91 + 95;

    float yMin = center.height - 87.5;
    float yMax = center.height + 87.5;

    if (pos.x < xMin || pos.x > xMax) return stopHovering();
    if (pos.y < yMin || pos.y > yMax) return stopHovering();

    for (int i = 1; i < 7; i++) {
        if (pos.y > yMax - (i * 29))
            return hoverInput(i - 1);
    }

    stopHovering();
}

bool MacroEditLayer::setup() {
    Utils::setBackgroundColor(m_bgSprite);
    
    CCMenu* menu = CCMenu::create();
    menu->setID("main-menu");
    m_mainLayer->addChild(menu);

    selectedInputMenu = CCMenu::create();
    selectedInputMenu->setID("selected-input-menu");
    m_mainLayer->addChild(selectedInputMenu);

    CCLabelBMFont* lbl = CCLabelBMFont::create("Edit Macro", "goldFont.fnt");
    lbl->setPosition({117, 104});
    lbl->setScale(0.55f);
    menu->addChild(lbl);

    inputCountLbl = CCLabelBMFont::create("Inputs: 156", "chatFont.fnt");
    inputCountLbl->setPosition({-187, 112});
    inputCountLbl->setAnchorPoint({0, 0.5});
    inputCountLbl->setOpacity(99);
    inputCountLbl->setScale(0.45f);
    menu->addChild(inputCountLbl);

    notSavedLabel = CCLabelBMFont::create("*", "chatFont.fnt");
    notSavedLabel->setPosition({-3, 94});
    notSavedLabel->setScale(0.7f);
    notSavedLabel->setOpacity(139);
    notSavedLabel->setVisible(false);
    menu->addChild(notSavedLabel);

    noInputsLabel1 = CCLabelBMFont::create("Empty", "bigFont.fnt");
    noInputsLabel1->setOpacity(174);
    noInputsLabel1->setPositionX(-91);
    noInputsLabel1->setScale(0.85f);
    menu->addChild(noInputsLabel1);

    noInputsLabel2 = CCLabelBMFont::create("No Input", "bigFont.fnt");
    noInputsLabel2->setOpacity(174);
    noInputsLabel2->setPositionX(117);
    noInputsLabel2->setScale(0.625f);
    menu->addChild(noInputsLabel2);

    hoveredBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    hoveredBg->setColor({ 255, 255, 255 });
    hoveredBg->setOpacity(44);
    hoveredBg->setScale(0.375f);
    hoveredBg->setPositionX(-91);
    hoveredBg->setAnchorPoint({ 0.5, 0.5 });
    hoveredBg->setContentSize({ 471, 65 });
    hoveredBg->setVisible(false);
    menu->addChild(hoveredBg);

    selectedBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    selectedBg->setColor({ 232, 255, 0 });
    selectedBg->setOpacity(44);
    selectedBg->setScale(0.375f);
    selectedBg->setPositionX(-91);
    selectedBg->setAnchorPoint({ 0.5, 0.5 });
    selectedBg->setContentSize({ 471, 65 });
    menu->addChild(selectedBg);

    listBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    listBg->setColor({ 0,0,0 });
    listBg->setOpacity(78);
    listBg->setPositionX(-91);
    listBg->setAnchorPoint({ 0.5, 0.5 });
    listBg->setContentSize({ 194, 209 });
    menu->addChild(listBg);

    selectedInputBg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    selectedInputBg->setColor({ 0,0,0 });
    selectedInputBg->setOpacity(78);
    selectedInputBg->setPosition({117, 4});
    selectedInputBg->setAnchorPoint({ 0.5, 0.5 });
    selectedInputBg->setContentSize({ 151, 167 });
    menu->addChild(selectedInputBg);

    CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    spr->setScale(0.55f);
    pageLeftBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchPage));
    pageLeftBtn->setID("left");
    pageLeftBtn->setPositionX(-199);
    menu->addChild(pageLeftBtn);

    spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
    spr->setFlipX(true);
    spr->setScale(0.55f);
    pageRightBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchPage));
    pageRightBtn->setPositionX(17);
    menu->addChild(pageRightBtn);

    ButtonSprite* btnSpr = ButtonSprite::create("Apply");
    btnSpr->setScale(0.5f);
    saveBtn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MacroEditLayer::onSave));
    saveBtn->setPosition({185, -99});
    menu->addChild(saveBtn);

    btnSpr = ButtonSprite::create("Merge");
    btnSpr->setScale(0.5f);
    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MacroEditLayer::onMerge));
    btn->setPosition({72, -99});
    menu->addChild(btn);

    btnSpr = ButtonSprite::create("Clear");
    btnSpr->setScale(0.5f);
    btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(MacroEditLayer::onClear));
    btn->setPosition({128.5, -99});
    menu->addChild(btn);

	spr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    spr->setScale(0.45f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::onAddInput));
    btn->setPosition({23.5, -94});
    menu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("GJ_deleteSongBtn_001.png");
    spr->setScale(0.55f);
    deleteBtn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::onRemoveInput));
    deleteBtn->setPosition({23.5, -64});
    menu->addChild(deleteBtn);

    lbl = CCLabelBMFont::create("Page", "bigFont.fnt");
    lbl->setPosition({23.5, 108});
    lbl->setScale(0.26f);
    menu->addChild(lbl);

    pageInput = TextInput::create(36, "0");
    pageInput->getInputNode()->setAllowedChars("0123456789");
    pageInput->getInputNode()->setMaxLabelLength(5);
    pageInput->getInputNode()->setDelegate(this);
    pageInput->setPosition({23.5, 91});
    pageInput->setScale(0.55f);
    menu->addChild(pageInput);

    float xPos = 57.f;
    float yPos = 44.5f;

    lbl = CCLabelBMFont::create("Selected Input", "bigFont.fnt");
    lbl->setPosition({117, 73});
    lbl->setOpacity(196);
    lbl->setScale(0.4f);
    menu->addChild(lbl);

    lbl = CCLabelBMFont::create("Frame: ", "bigFont.fnt");
    lbl->setScale(0.375f);
    lbl->setAnchorPoint({0, 0.5});
    lbl->setPosition({xPos, yPos});
    selectedInputMenu->addChild(lbl);

    frameInput = TextInput::create(70, "236");
    frameInput->setPosition({xPos + 91, yPos});
    frameInput->setScale(0.625f);
    frameInput->getInputNode()->setAllowedChars("0123456789");
    frameInput->getInputNode()->setMaxLabelLength(8);
    frameInput->getInputNode()->setDelegate(this);
    selectedInputMenu->addChild(frameInput);

    spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchFrame));
    btn->setPosition({xPos + 62, yPos});
    btn->setID("left");
    selectedInputMenu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchFrame));
    btn->setPosition({xPos + 120, yPos});
    selectedInputMenu->addChild(btn);

    lbl = CCLabelBMFont::create("Button: ", "bigFont.fnt");
    lbl->setAnchorPoint({0, 0.5});
    lbl->setScale(0.375f);
    lbl->setPosition({xPos, yPos - 35});
    selectedInputMenu->addChild(lbl);

    spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchButton));
    btn->setPosition({xPos + 62, yPos - 35});
    btn->setID("left");
    selectedInputMenu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchButton));
    btn->setPosition({xPos + 120, yPos - 35});
    selectedInputMenu->addChild(btn);
    
    CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0,0,0 });
    bg->setScale(0.3125f);
    bg->setOpacity(90);
    bg->setPosition({xPos + 91, yPos - 35});
    bg->setAnchorPoint({ 0.5, 0.5 });
    bg->setContentSize({ 140, 60 });
    selectedInputMenu->addChild(bg);

    buttonLabel = CCLabelBMFont::create("Left", "bigFont.fnt");
    buttonLabel->setScale(0.375f);
    buttonLabel->setPosition({xPos + 91, yPos - 35});
    selectedInputMenu->addChild(buttonLabel);

    lbl = CCLabelBMFont::create("Action: ", "bigFont.fnt");
    lbl->setAnchorPoint({0, 0.5});
    lbl->setScale(0.375f);
    lbl->setPosition({xPos, yPos - 66});
    selectedInputMenu->addChild(lbl);

    spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchAction));
    btn->setPosition({xPos + 62, yPos - 66});
    selectedInputMenu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchAction));
    btn->setPosition({xPos + 120, yPos - 66});
    selectedInputMenu->addChild(btn);
    
    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0,0,0 });
    bg->setScale(0.3125f);
    bg->setOpacity(90);
    bg->setPosition({xPos + 91, yPos - 66});
    bg->setAnchorPoint({ 0.5, 0.5 });
    bg->setContentSize({ 140, 60 });
    selectedInputMenu->addChild(bg);

    actionLabel = CCLabelBMFont::create("Hold", "bigFont.fnt");
    actionLabel->setScale(0.375f);
    actionLabel->setPosition({xPos + 91, yPos - 66});
    selectedInputMenu->addChild(actionLabel);

    lbl = CCLabelBMFont::create("Player: ", "bigFont.fnt");
    lbl->setAnchorPoint({0, 0.5});
    lbl->setScale(0.375f);
    lbl->setPosition({xPos, yPos - 97});
    selectedInputMenu->addChild(lbl);

    spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchPlayer));
    btn->setPosition({xPos + 62, yPos - 97});
    selectedInputMenu->addChild(btn);

    spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
    spr->setScale(0.55f);
    btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(MacroEditLayer::switchPlayer));
    btn->setPosition({xPos + 120, yPos - 97});
    selectedInputMenu->addChild(btn);

    bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
    bg->setColor({ 0,0,0 });
    bg->setScale(0.3125f);
    bg->setOpacity(90);
    bg->setPosition({xPos + 91, yPos - 97});
    bg->setAnchorPoint({ 0.5, 0.5 });
    bg->setContentSize({ 140, 60 });
    selectedInputMenu->addChild(bg);

    playerLabel = CCLabelBMFont::create("One", "bigFont.fnt");
    playerLabel->setScale(0.375f);
    playerLabel->setPosition({xPos + 91, yPos - 97});
    selectedInputMenu->addChild(playerLabel);

    for (int i = 0; i < 7; i++) {
        CCLabelBMFont* lbl = CCLabelBMFont::create("_____________________", "chatFont.fnt");
        lbl->setPosition(ccp(-91, 93.5 - (i * 29)));
        lbl->setColor(ccc3(0, 0, 0));
        lbl->setOpacity(80);
        menu->addChild(lbl);
        separators.push_back(lbl);
    }

    inputs = Global::get().macro.inputs;
    ogInputs = inputs;

    loadPage(1);

    hoveredInput = -2;
    selectInput(0);

    updateSaved();

    editLayer = this;

    cocos2d::CCPoint offset = (CCDirector::sharedDirector()->getWinSize() - m_mainLayer->getContentSize()) / 2;
    m_mainLayer->setPosition(m_mainLayer->getPosition() - offset);
    m_closeBtn->setPosition(m_closeBtn->getPosition() + offset);
    m_bgSprite->setPosition(m_bgSprite->getPosition() + offset);
    
    return true;
}

void MacroEditLayer::loadPage(int page) {
    currentPage = page;

    pageInput->setString(std::to_string(currentPage).c_str());

    if (pageMenu)
        pageMenu->removeFromParentAndCleanup(true);
    
    pageMenu = CCMenu::create();
    pageMenu->setID("page-menu");
    m_mainLayer->addChild(pageMenu);

    updatePageInputs(page);

    bool empty = pageInputs.empty();

    noInputsLabel1->setVisible(empty); 
    noInputsLabel2->setVisible(empty);

    selectedInputMenu->setVisible(!empty);

    bool disableArrows = inputs.size() <= 6;

    deleteBtn->getChildByType<CCSprite>(0)->setOpacity(empty ? 130 : 255);
    deleteBtn->setEnabled(!empty);

    pageLeftBtn->getChildByType<CCSprite>(0)->setOpacity((disableArrows && !empty) || inputs.empty() ? 130 : 255);
    pageLeftBtn->setEnabled(!((disableArrows && !empty) || inputs.empty()));

    disableArrows = disableArrows || (empty && !inputs.empty());

    pageRightBtn->getChildByType<CCSprite>(0)->setOpacity(disableArrows ? 130 : 255);
    pageRightBtn->setEnabled(!disableArrows);

    for (int i = 0; i < 7; i++)
        separators[i]->setVisible(i <= pageInputs.size() && !empty);

    if (empty) {
        selectedBg->setVisible(false);
        hoveredBg->setVisible(false);

        selectedInput = -1;
        selectedInputIndex = -1;
        hoveredInput = -1;

        return;
    }

    if (selectedInput == -1)
        selectInput(pageInputs.size() - 1);

    for (int i = 0; i < pageInputs.size(); i ++) {
        float height1 = 78.f - (i * 29.f);
        float height2 = 67.f - (i * 29.f);

        InputText inp = getInputText(pageInputs[i]);

        CCLabelBMFont* lbl = CCLabelBMFont::create("Frame", "chatFont.fnt");
        lbl->setScale(0.525f);
        lbl->setOpacity(134);
        lbl->setPosition({-153, height1});

        pageMenu->addChild(lbl);
        
        lbl = CCLabelBMFont::create(inp.frame.c_str(), "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setOpacity(218);
        lbl->setPosition({-153, height2});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create("Button", "chatFont.fnt");
        lbl->setScale(0.525f);
        lbl->setOpacity(134);
        lbl->setPosition({-112, height1});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create(inp.button.c_str(), "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setOpacity(218);
        lbl->setPosition({-112, height2});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create("Action", "chatFont.fnt");
        lbl->setScale(0.525f);
        lbl->setOpacity(134);
        lbl->setPosition({-69, height1});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create(inp.action.c_str(), "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setOpacity(218);
        lbl->setPosition({-69, height2});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create("Player", "chatFont.fnt");
        lbl->setScale(0.525f);
        lbl->setOpacity(134);
        lbl->setPosition({-31, height1});

        pageMenu->addChild(lbl);

        lbl = CCLabelBMFont::create(inp.player.c_str(), "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setOpacity(218);
        lbl->setPosition({-31, height2});

        pageMenu->addChild(lbl);
    }

}

InputText MacroEditLayer::getInputText(input input) {
    InputText ret = {
        std::to_string(input.frame),
        btnNames.contains(input.button) ? btnNames.at(input.button) : std::to_string(input.button),
        input.player2 ? "Two" : "One",
        input.down ? "Hold" : "Release"
    };

    return ret;
}

void MacroEditLayer::textChanged(CCTextInputNode* input) {
    if (!input || !editLayer) return;

    std::string text = input->getString();

    if (text == "") return;

    if (input == editLayer->pageInput->getInputNode()) {
        int page = std::stoi(text);

        if (page <= 0) return;

        editLayer->loadPage(page);
        editLayer->reSelectInput();

        return;
    }

    if (input == editLayer->frameInput->getInputNode()) {
        int frame = std::stoi(text);

        editLayer->changeSelectedInputFrame(frame, false);

        return;
    }
}

void MacroEditLayer::updatePageInputs(int page) {

    if (!pageInputs.empty())
        pageInputs.clear();

    for (int i = 0; i < 6; i++) {
        int index = (page - 1) * 6 + i;

        if (index >= inputs.size()) break;

        pageInputs.push_back(inputs[index]);
    }
}

void MacroEditLayer::hoverInput(int input) {
    if (input == selectedInput) return stopHovering();
    if (input >= pageInputs.size()) return stopHovering();

    float y = 72.5f - (29 * input);
    hoveredBg->setPositionY(y);
    hoveredBg->setVisible(true);

    hoveredInput = input;
}

void MacroEditLayer::stopHovering() {
    hoveredBg->setVisible(false);
    hoveredInput = -1;
}

void MacroEditLayer::updateLabels() {
    auto currentInput = inputs[selectedInputIndex];
    InputText inp = getInputText(currentInput);

    frameInput->setString(inp.frame.c_str());
    playerLabel->setString(inp.player.c_str());
    buttonLabel->setString(inp.button.c_str());

    actionLabel->setString(inp.action.c_str());
    actionLabel->limitLabelWidth(33.f, 1.f, 0.01f);
    actionLabel->updateLabel();
}

void MacroEditLayer::selectInput(int input) {
    if (hoveredInput == -1 || hoveredInput == selectedInput || selectedInput == input) return;

    if (hoveredInput == -2)
        hoveredInput = -1;

    if (pageInputs.empty()) return;

    if (input >= pageInputs.size())
        input = pageInputs.size() - 1;

    float y = 72.5f - (29 * input);
    selectedBg->setPositionY(y);
    selectedBg->setVisible(true);

    selectedInput = input;
    selectedInputIndex = (currentPage - 1) * 6 + input;
    
    updateLabels();    
    flashSelected();
}

void MacroEditLayer::reSelectInput() {
    int prevSelected = selectedInput;

    selectedInput = -1;
    selectedInputIndex = -1;
    hoveredInput = -2;

    selectInput(prevSelected);
}

int MacroEditLayer::getSum(CCObject* obj) {
    if (std::string_view(static_cast<CCNode*>(obj)->getID()) == "left")
        return -1;
        
    return 1;
}

void MacroEditLayer::switchPage(CCObject* obj) {
    int sum = getSum(obj);

    currentPage += sum;

    int lastPage = inputs.size() / 6;
    bool empty = pageInputs.empty();

    if (inputs.size() != lastPage * 6)
        lastPage++;

    if (currentPage == 0 || empty) {
        currentPage = lastPage;
    }
    else if (currentPage >= lastPage + 1) {
        currentPage = 1;
    }

    loadPage(currentPage);
    reSelectInput();
    
    if (empty)
        selectInput(5);
}

void MacroEditLayer::switchFrame(CCObject* obj) {
    auto input = inputs[selectedInputIndex];
    
    int sum = getSum(obj);

    if (input.frame == 0 && sum == -1) return;

    changeSelectedInputFrame(input.frame + sum);
}

void MacroEditLayer::changeSelectedInputFrame(int frame, bool isArrow) {
    auto input = inputs[selectedInputIndex];

    input.frame = frame;
    
    inputs.erase(inputs.begin() + selectedInputIndex);
    
    size_t addedAt = 0;

    for (size_t i = 0; i < inputs.size(); i++) {
        int currentFrame = inputs[i].frame;
        int nextFrame = i != inputs.size() - 1 ? inputs[i + 1].frame : -1;

        if ((selectedInputIndex == 0 && input.frame < currentFrame) || frame <= inputs[0].frame) {
            inputs.insert(inputs.begin(), input);

            addedAt = 0;

            break;
        }

        if (selectedInputIndex == inputs.size() && isArrow) {

            addedAt = selectedInputIndex;
            inputs.push_back(input);

            auto secLast = inputs.size() - 2;

            if (inputs[secLast].frame > input.frame) {
                auto inputCopy = inputs[secLast];

                inputs.erase(inputs.begin() + secLast);
                inputs.push_back(inputCopy);

                addedAt = selectedInputIndex - 1;
            }

            break;
        }

        if ((input.frame > currentFrame && input.frame <= nextFrame) || nextFrame == -1) {

            inputs.insert(inputs.begin() + i + 1, input);
            
            addedAt = i + 1;

            break;
        }
    }

    int co = addedAt / 6;
    int newPage = co + 1;
    int newSelected = addedAt - co * 6;

    loadPage(newPage);

    selectedInput = newSelected;
    reSelectInput();
    updateSaved();
}

void MacroEditLayer::switchButton(CCObject* obj) {
    int sum = getSum(obj);

    auto& input = inputs[selectedInputIndex];
    
    input.button += sum;

    if (input.button == 0) input.button = 3;   
    if (input.button == 4) input.button = 1; 

    loadPage(currentPage);
    reSelectInput();
    updateSaved();
}

void MacroEditLayer::switchAction(CCObject* obj) {
    auto& input = inputs[selectedInputIndex];

    input.down = !input.down;

    loadPage(currentPage);
    reSelectInput();
    updateSaved();
}

void MacroEditLayer::switchPlayer(CCObject* obj) {
    auto& input = inputs[selectedInputIndex];

    input.player2 = !input.player2;

    loadPage(currentPage);
    reSelectInput();
    updateSaved();
}

void MacroEditLayer::flashSelected() {
    if (!selectedBg->isVisible()) return;

    CCTintTo* tintTo = CCTintTo::create(0.05f, 247, 255, 140);
    CCTintTo* tintFrom = CCTintTo::create(0.35f, 232, 255, 0);

    CCFadeTo* fadeTo = CCFadeTo::create(0.05f, 100);
    CCFadeTo* fadeFrom = CCFadeTo::create(0.35f, 44);

    CCSequence* colorSeq = CCSequence::create(tintTo, tintFrom, nullptr);
    CCSequence* opacitySeq = CCSequence::create(fadeTo, fadeFrom, nullptr);

    CCSpawn* spawn = CCSpawn::create(colorSeq, opacitySeq, nullptr);

    selectedBg->runAction(spawn);

    tintTo = CCTintTo::create(0.05f, 20, 20, 20);
    tintFrom = CCTintTo::create(0.35f, 0, 0, 0);

    selectedInputBg->runAction(CCSequence::create(tintTo, tintFrom, nullptr));
}

void MacroEditLayer::onAddInput(CCObject*) {
    if (inputs.empty()) {
        inputs.push_back({1, 1, false, true});
        loadPage(1);

        hoveredInput = -2;
        selectInput(0);

        return updateSaved();
    }

    if (pageInputs.empty()) {
        int lastPage = inputs.size() / 6;

        if (inputs.size() != lastPage * 6)
            lastPage++;

        loadPage(lastPage);

        if (pageInputs.empty()) return updateSaved();

        hoveredInput = -2;
        selectInput(pageInputs.size() - 1);
    }

    auto input = inputs[selectedInputIndex];
    input.frame++;

    inputs.insert(inputs.begin() + (selectedInputIndex + 1), input);

    if (selectedInput == 5) {
        loadPage(currentPage + 1);

        hoveredInput = -2;
        selectInput(0);
        flashSelected();

        return updateSaved();
    }

    hoveredInput = -2;
    loadPage(currentPage);
    selectInput(selectedInput + 1);
    updateSaved();
}

void MacroEditLayer::onRemoveInput(CCObject*) {
    if (inputs.empty() || pageInputs.empty() || selectedInput == -1) return;

    inputs.erase(inputs.begin() + selectedInputIndex);

    loadPage(currentPage);
    reSelectInput();

    while (pageInputs.empty() && currentPage != 1) {
        loadPage(currentPage - 1);
        selectedInput != 5 ? selectInput(5) : reSelectInput();
    }

    if (inputs.empty()) {
        CCTintTo* tintTo = CCTintTo::create(0.05f, 20, 20, 20);
        CCTintTo* tintFrom = CCTintTo::create(0.35f, 0, 0, 0);

        selectedInputBg->runAction(CCSequence::create(tintTo, tintFrom, nullptr));
        listBg->runAction(CCSequence::create(tintTo, tintFrom, nullptr));
    }
    
    updateSaved();
}

void MacroEditLayer::onSave(CCObject*) {
    std::string extension = Global::get().macro.frameFixes.empty() || inputs.empty() ? "." : " \n<cr>All Frame Fixes will be removed</c>.";

    geode::createQuickPopup(
        "Apply",
        "Apply <cl>changes</c> to current macro?" + extension,
        "Cancel", "Yes",
        [this](auto, bool btn2) {
            if (!btn2) return;

            auto& g = Global::get();
            g.macro.frameFixes.clear();
            g.macro.inputs = this->inputs;

            this->saved = true;

            onClose(nullptr);

            CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
            CCObject* child;
            CCARRAY_FOREACH(children, child) {
                if (RecordLayer* layer = typeinfo_cast<RecordLayer*>(child)) {
                    layer->keyBackClicked();
                    break;
                }
            }

            RecordLayer::openMenu(true);
            MacroEditLayer::open(true);

            Loader::get()->queueInMainThread([] {
                CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
                CCObject* child;
                CCARRAY_FOREACH(children, child) {
                    if (MacroEditLayer* layer = typeinfo_cast<MacroEditLayer*>(child)) {
                        editLayer = layer;
                        break;
                    }
                }
            });

            Notification::create("Applied", NotificationIcon::Success)->show();
        }
    );
}

void MacroEditLayer::toggleSaveButton(bool toggle) {
    ButtonSprite* btnSpr = saveBtn->getChildByType<ButtonSprite>(0);
    CCScale9Sprite* spr = btnSpr->getChildByType<CCScale9Sprite>(0);
    CCLabelBMFont* lbl = btnSpr->getChildByType<CCLabelBMFont>(0);

    saveBtn->setEnabled(toggle);
    spr->setOpacity(toggle ? 255 : 130);
    lbl->setOpacity(toggle ? 255 : 130);
}

void MacroEditLayer::updateSaved() {
    saved = inputs == ogInputs;

    notSavedLabel->setVisible(!saved);
    saveBtn->setEnabled(!saved);
    toggleSaveButton(!saved);

    inputCountLbl->setString(fmt::format("Inputs: {}", inputs.size()).c_str());
}

void MacroEditLayer::onClear(CCObject*) {
    auto& g = Global::get();
    if (inputs.empty()) return;

    geode::createQuickPopup(
        "Clear",
        "Clear <cy>" + std::to_string(inputs.size()) + "</c> macro actions?",
        "Cancel", "Yes",
        [this](auto, bool btn2) {
            if (btn2) {
                inputs.clear();
                loadPage(1);
                updateSaved();
            }
        }
    );
}

void MacroEditLayer::onMerge(CCObject*) {
    geode::Popup<>* layer = nullptr;
    if (Global::get().layer)
        layer = typeinfo_cast<geode::Popup<>*>(Global::get().layer);
    else {
        CCArray* children = CCDirector::sharedDirector()->getRunningScene()->getChildren();
        CCObject* child;
        CCARRAY_FOREACH(children, child) {
            if (typeinfo_cast<RecordLayer*>(child)) {
                layer = typeinfo_cast<geode::Popup<>*>(child);
                break;
            }
        }
    }

    if (!layer) {
        onClose(nullptr);
        return;
    }

    LoadMacroLayer::open(layer, this, false);
}

void MacroEditLayer::mergeMacro(std::vector<input> mergeInputs, bool players[2], bool overwrite) {
    if (mergeInputs.empty()) return;

    std::vector<input> actualInputs;
    int endFrame[2] = {0, 0};

    for (input inp : mergeInputs) {
        if (players[static_cast<int>(inp.player2)])
            actualInputs.push_back(inp);
    }

    for (int i = 0; i < 2; i++) {
        for (int j = mergeInputs.size() - 1; j >= 0; j--) {
            if (!players[i]) break;
            input inp = mergeInputs[j];
            if (static_cast<int>(inp.player2) != i) continue;
            endFrame[i] = inp.frame;
            break;
        }
    }

    if (overwrite) {
        int startFrame[2] = {0, 0};

        for (int i = 0; i < 2; i++) {
            for (input inp : mergeInputs) {
                if (!players[i]) break;
                if (static_cast<int>(inp.player2) != i) continue;
                startFrame[i] = inp.frame;
                break;
            }
        }

        auto condition = [startFrame, endFrame, players](const input& element) {
            int p = static_cast<int>(element.player2);
            return element.frame >= startFrame[p] && element.frame <= endFrame[p] && players[p];
        };
    
        auto newEnd = std::remove_if(inputs.begin(), inputs.end(), condition);
    
        inputs.erase(newEnd, inputs.end());
    }

    std::vector<input> mergedVector;
    mergedVector.resize(inputs.size() + actualInputs.size());
    std::merge(actualInputs.begin(), actualInputs.end(), inputs.begin(), inputs.end(), mergedVector.begin());

    inputs = mergedVector;

    for (int j = inputs.size() - 1; j >= 0; j--) {
        if (inputs[j].frame >= endFrame[0]) continue;

        int co = j / 6;
        int newPage = co + 1;
        int newSelected = j - co * 6;

        loadPage(newPage);

        selectedInput = -1;
        hoveredInput = -2;
        selectInput(newSelected + (newSelected < 5 ? 1 : 0));

        break;
    }
        
    updateSaved();
}