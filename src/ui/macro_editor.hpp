#pragma once

#include "../includes.hpp"

const std::unordered_map<int, std::string> btnNames = { {1, "Jump"}, {2, "Left"}, {3, "Right"},{4, "Jump"}, {5, "Left"}, {6, "Right"} };

struct InputText {
    std::string frame;
    std::string button;
    std::string player;
    std::string action;
};

class MacroEditLayer : public geode::Popup<>, public TextInputDelegate {

private:
    
    bool setup();

    void onClose(CCObject*) override;

    void keyBackClicked() override {
        onClose(nullptr);
    }

    void textChanged(CCTextInputNode* input) override;

    ~MacroEditLayer() override;
    
public:

    STATIC_CREATE(MacroEditLayer, 438, 247)
    
    TextInput* pageInput = nullptr;
    TextInput* frameInput = nullptr;
    CCLabelBMFont* actionLabel = nullptr;
    CCLabelBMFont* buttonLabel = nullptr;
    CCLabelBMFont* playerLabel = nullptr;
    CCLabelBMFont* noInputsLabel1 = nullptr;
    CCLabelBMFont* noInputsLabel2 = nullptr;
    CCLabelBMFont* notSavedLabel = nullptr;
    CCLabelBMFont* inputCountLbl = nullptr;

    std::vector<CCLabelBMFont*> separators;
    std::vector<input> ogInputs;
    std::vector<input> inputs;
    std::vector<input> pageInputs;

    CCMenu* pageMenu = nullptr;
    CCMenu* selectedInputMenu = nullptr;

    CCMenuItemSpriteExtra* saveBtn = nullptr;
    CCMenuItemSpriteExtra* deleteBtn = nullptr;
    CCMenuItemSpriteExtra* pageLeftBtn = nullptr;
    CCMenuItemSpriteExtra* pageRightBtn = nullptr;

    CCScale9Sprite* hoveredBg = nullptr;
    CCScale9Sprite* selectedBg = nullptr;
    CCScale9Sprite* selectedInputBg = nullptr;
    CCScale9Sprite* listBg = nullptr;

    int selectedInput = -1;
    int selectedInputIndex = -1;
    int hoveredInput = -1;

    int currentPage = 1;

    bool saved = true;

    void loadPage(int page);

    void updateHover(cocos2d::CCPoint pos);

    void hoverInput(int input);

    void stopHovering();

    void selectInput(int input);

    void reSelectInput();

    void updatePageInputs(int page);

    void updateLabels();

    void switchPage(CCObject* obj);
    
    void switchFrame(CCObject* obj);

    void switchButton(CCObject* obj);

    void switchAction(CCObject* obj);

    void switchPlayer(CCObject* obj);

    void flashSelected();

    void onAddInput(CCObject*);

    void onRemoveInput(CCObject*);

    void changeSelectedInputFrame(int frame, bool isArrow = true);

    void onSave(CCObject*);

    void onClear(CCObject*);

    void onMerge(CCObject*);

    void mergeMacro(std::vector<input> mergeInputs, bool players[2], bool overwrite);

    void updateSaved();

    void toggleSaveButton(bool toggle);

    static int getSum(CCObject* obj);

    static InputText getInputText(input input);

    static void open(bool instant = false) {
        MacroEditLayer* layer = create();
        layer->m_noElasticity = instant;
        layer->show();
    }

};