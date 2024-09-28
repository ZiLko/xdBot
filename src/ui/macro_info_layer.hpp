#pragma once
#include "../includes.hpp"
#include <Geode/ui/MDTextArea.hpp>

class MacroInfoLayer : public geode::Popup<> {

public:

    static MacroInfoLayer* create() {
        MacroInfoLayer* ret = new MacroInfoLayer();
        if (ret->initAnchored(417, 268, "square01_001.png", CCRectZero)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

private:

    bool setup() override {
        setTitle("Current Macro");
        auto& g = Global::get();

        int playerInputs[2][3][2] = { { { 0, 0 }, { 0, 0 }, { 0, 0 } }, { { 0, 0 }, { 0, 0 }, { 0, 0 } } };

        for (const auto& input : g.macro.inputs)
            playerInputs[input.player2][input.button - 1][input.down]++;

        CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setPosition(ccp(29, 225));
        bg->setAnchorPoint({ 0, 1 });
        bg->setContentSize({ 162, 202 });
        m_mainLayer->addChild(bg);

        bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setPosition(ccp(226, 225));
        bg->setAnchorPoint({ 0, 1 });
        bg->setContentSize({ 162, 202 });
        m_mainLayer->addChild(bg);

        CCLabelBMFont* lbl = CCLabelBMFont::create("General Info", "goldFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 108, 211 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Inputs Info", "goldFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 211 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create(g.macro.author.c_str(), "chatFont.fnt");
        lbl->setScale(0.7f);
        lbl->setPosition({ 108, 184 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("____________________________", "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 108, 177 });
        lbl->setOpacity(136);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Author", "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({ 108, 166 });
        lbl->setOpacity(137);
        m_mainLayer->addChild(lbl);

        MDTextArea* desc = MDTextArea::create((g.macro.description == "" ? "N/A" : g.macro.description).c_str(), { 140, 30 });
        desc->setPosition({ 108, 141 });
        m_mainLayer->addChild(desc);

        lbl = CCLabelBMFont::create("Description", "chatFont.fnt");
        lbl->setScale(0.5f);
        lbl->setPosition({ 108, 118 });
        lbl->setOpacity(137);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Framerate:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 100 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Duration:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 89 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Game version:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 78 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Seed:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 67 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Used LDM:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 56 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Level:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 45 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Bot:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 36, 34 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create(std::to_string(g.macro.framerate).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(221.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 76, 100 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        std::string duration = g.macro.inputs.empty() ? std::string("0s") : (std::to_string(g.macro.inputs.back().frame / g.macro.framerate) + "s");

        lbl = CCLabelBMFont::create(duration.c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(231.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 70.6, 89 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        std::string version = g.macro.gameVersion != 0 ? std::to_string(g.macro.gameVersion) : std::string("N/A");

        lbl = CCLabelBMFont::create(version.c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(198.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 88, 78 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        std::string seed = g.macro.seed != 0 ? std::to_string(g.macro.seed) : std::string("N/A");

        lbl = CCLabelBMFont::create(seed.c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(257.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 58, 67 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create(g.macro.ldm ? "true" : "false", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.475f);
        lbl->setPosition({ 75.5, 56 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        std::string levelName = g.macro.levelInfo.name != "" ? g.macro.levelInfo.name : "N/A";
        std::string levelID = g.macro.levelInfo.id != 0 ? std::to_string(g.macro.levelInfo.id) : "";

        lbl = CCLabelBMFont::create(fmt::format("{} ({})", levelName, levelID).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(257.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 59.3, 45 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((g.macro.botInfo.name + " " + g.macro.botInfo.version).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->limitLabelWidth(271.f, 0.475, 0.01f);
        lbl->updateLabel();
        lbl->setPosition({ 52.3, 34 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Total actions:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 189 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("____________________________", "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 177 });
        lbl->setOpacity(136);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("P1 Holds / Releases", "chatFont.fnt");
        lbl->setAnchorPoint({ 0.5, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 163 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("____________________________", "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 156 });
        lbl->setOpacity(136);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Click:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 138 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Left:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 125 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Right:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 112 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("____________________________", "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 101 });
        lbl->setOpacity(136);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("P2 Holds / Releases", "chatFont.fnt");
        lbl->setAnchorPoint({ 0.5, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 87 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("____________________________", "chatFont.fnt");
        lbl->setScale(0.55f);
        lbl->setPosition({ 308, 80 });
        lbl->setOpacity(136);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Click:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 62 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Left:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 49 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create("Right:", "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 233, 36 });
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create(std::to_string(g.macro.inputs.size()).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 286, 189 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[0][0][0]) + " / " + std::to_string(playerInputs[0][0][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 258.5, 138 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[0][1][0]) + " / " + std::to_string(playerInputs[0][1][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 256, 125 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[0][2][0]) + " / " + std::to_string(playerInputs[0][2][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 260, 112 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[1][0][0]) + " / " + std::to_string(playerInputs[1][0][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 258.5, 62 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[1][1][0]) + " / " + std::to_string(playerInputs[1][1][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 256, 49 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        lbl = CCLabelBMFont::create((std::to_string(playerInputs[1][2][0]) + " / " + std::to_string(playerInputs[1][2][1])).c_str(), "chatFont.fnt");
        lbl->setAnchorPoint({ 0, 0.5 });
        lbl->setScale(0.55f);
        lbl->setPosition({ 260, 36 });
        lbl->setOpacity(150);
        m_mainLayer->addChild(lbl);

        return true;
    }

};