#include "../includes.hpp"
#include "record_layer.hpp"

class RenderPresetsLayer : public geode::Popup<> {

public:

	STATIC_CREATE(RenderPresetsLayer, 243, 231)

private:

    bool setup() override {
        setTitle("Render Presets");

        CCScale9Sprite* bg = CCScale9Sprite::create("square02b_001.png", { 0, 0, 80, 80 });
        bg->setColor({ 0,0,0 });
        bg->setOpacity(75);
        bg->setPosition({m_size.width / 2, 116.5});
        bg->setContentSize({ 213, 151 });
        m_mainLayer->addChild(bg);

        for (int i = 0; i < 5; i++) {
            float height = 170.f - 26.5f * i;
            std::string id = "render_slot_" + std::to_string(i + 1);

            CCLabelBMFont* lbl = CCLabelBMFont::create(("Slot " + std::to_string(i + 1)).c_str(), "bigFont.fnt");
            lbl->setPosition({63, height + 5.f});
            lbl->setScale(0.425f);
            m_mainLayer->addChild(lbl);

            lbl = CCLabelBMFont::create("1920 x 1080", "chatFont.fnt");
            lbl->setPosition({63, height - 6.f});
            lbl->setScale(0.4f);
            lbl->setOpacity(105);
            m_mainLayer->addChild(lbl);

            ButtonSprite* spr = ButtonSprite::create("Load");
            spr->setScale(0.6f);
            CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RenderPresetsLayer::onLoad));
            btn->setPosition({123, height});
            btn->setID(id.c_str());
            m_buttonMenu->addChild(btn);

            if (!Mod::get()->hasSavedValue(id)) {
                lbl->setString("N/A");
                btn->setEnabled(false);
                spr->getChildByType<CCScale9Sprite>(0)->setOpacity(120);
                spr->getChildByType<CCLabelBMFont>(0)->setOpacity(120);
            } else {
                btn->setEnabled(true);
                matjson::Value json = Mod::get()->getSavedValue<matjson::Value>(id);
                lbl->setString((json["width"].asString().unwrapOrDefault() + " x " + json["height"].asString().unwrapOrDefault()).c_str());
            }


            spr = ButtonSprite::create("Save");
            spr->setScale(0.6f);
            btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(RenderPresetsLayer::onSave));
            btn->setPosition({183, height});
            btn->setID(id.c_str());
            m_buttonMenu->addChild(btn);
        }

        ButtonSprite* btnSpr = ButtonSprite::create("OK");
        btnSpr->setScale(0.75f);
        CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(btnSpr, this, menu_selector(RenderPresetsLayer::onClose));
        btn->setPosition({m_size.width / 2, 22});
        m_buttonMenu->addChild(btn);

        CCSprite* spr = CCSprite::createWithSpriteFrameName("GJ_plainBtn_001.png");
        spr->setScale(0.525f);

        CCSprite* spr2 = CCSprite::createWithSpriteFrameName("folderIcon_001.png");
        spr2->setPosition(spr->getContentSize() / 2);
        spr2->setScale(0.7f);

        spr->addChild(spr2);
        btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(RenderPresetsLayer::openRendersFolder)
        );
        btn->setPosition({18, 18});
        m_buttonMenu->addChild(btn);

        return true;
    }

    void update() {
        onClose(nullptr);

        Loader::get()->queueInMainThread([] {
            CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
            if (RecordLayer* layer = scene->getChildByType<RecordLayer>(0))
                layer->onClose(nullptr);
            RecordLayer::openMenu(true);
            
            Loader::get()->queueInMainThread([] {
                RenderPresetsLayer* layer = RenderPresetsLayer::create();
                layer->m_noElasticity = true;
                layer->show();
            });
        });
    }

    void openRendersFolder(CCObject*) {
        std::filesystem::path path = Mod::get()->getSettingValue<std::filesystem::path>("render_folder");

        if (std::filesystem::exists(path))
            file::openFolder(path);
        else if (std::filesystem::create_directory(path))
            file::openFolder(path);
        else
            FLAlertLayer::create("Error", "There was an error getting the folder. ID: 4", "Ok")->show();
    }

    void onLoad(CCObject* obj) {
        std::string id = static_cast<CCNode*>(obj)->getID();

        Mod* m = Mod::get();
        if (!m->hasSavedValue(id)) return;

        matjson::Value json = m->getSavedValue<matjson::Value>(id);

        m->setSavedValue("render_width2", json["width"].asString().unwrapOrDefault());
        m->setSavedValue("render_height", json["height"].asString().unwrapOrDefault());
        m->setSavedValue("render_bitrate", json["bitrate"].asString().unwrapOrDefault());
        m->setSavedValue("render_fps", json["fps"].asString().unwrapOrDefault());
        m->setSavedValue("render_codec", json["codec"].asString().unwrapOrDefault());
        m->setSavedValue("render_args", json["args"].asString().unwrapOrDefault());
        m->setSavedValue("render_video_args", json["video_args"].asString().unwrapOrDefault());
        m->setSavedValue("render_audio_args", json["audio_args"].asString().unwrapOrDefault());
        m->setSavedValue("seconds_after", json["seconds_after"].asString().unwrapOrDefault());
        m->setSavedValue("render_fade_in_time", json["fade_in_time"].asString().unwrapOrDefault());
        m->setSavedValue("render_fade_out_time", json["fade_out_time"].asString().unwrapOrDefault());

        m->setSavedValue("render_only_song", json["only_song"].asBool().unwrapOrDefault());
        m->setSavedValue("render_record_audio", json["record_audio"].asBool().unwrapOrDefault());
        m->setSavedValue("render_hide_endscreen", json["hide_endscreen"].asBool().unwrapOrDefault());
        m->setSavedValue("render_hide_levelcomplete", json["hide_levelcomplete"].asBool().unwrapOrDefault());
        m->setSavedValue("render_fade_in", json["fade_in"].asBool().unwrapOrDefault());
        m->setSavedValue("render_fade_out", json["fade_out"].asBool().unwrapOrDefault());

        m->setSavedValue("render_sfx_volume", json["sfx_volume"].asDouble().unwrapOrDefault());
        m->setSavedValue("render_music_volume", json["music_volume"].asDouble().unwrapOrDefault());

        update();
    }

    void onSave(CCObject* obj) {
        std::string id = static_cast<CCNode*>(obj)->getID();
        Mod* m = Mod::get();
        matjson::Value json;

        json["width"] = m->getSavedValue<std::string>("render_width2");
        json["height"] = m->getSavedValue<std::string>("render_height");
        json["bitrate"] = m->getSavedValue<std::string>("render_bitrate");
        json["fps"] = m->getSavedValue<std::string>("render_fps");
        json["codec"] = m->getSavedValue<std::string>("render_codec");
        json["args"] = m->getSavedValue<std::string>("render_args");
        json["video_args"] = m->getSavedValue<std::string>("render_video_args");
        json["audio_args"] = m->getSavedValue<std::string>("render_audio_args");
        json["seconds_after"] = m->getSavedValue<std::string>("seconds_after");
        json["fade_in_time"] = m->getSavedValue<std::string>("render_fade_in_time");
        json["fade_out_time"] = m->getSavedValue<std::string>("render_fade_out_time");

        json["only_song"] = m->getSavedValue<bool>("render_only_song");
        json["record_audio"] = m->getSavedValue<bool>("render_record_audio");
        json["hide_endscreen"] = m->getSavedValue<bool>("render_hide_endscreen");
        json["hide_levelcomplete"] = m->getSavedValue<bool>("render_hide_levelcomplete");
        json["fade_in"] = m->getSavedValue<bool>("render_fade_in");
        json["fade_out"] = m->getSavedValue<bool>("render_fade_out");

        json["sfx_volume"] = m->getSavedValue<double>("render_sfx_volume");
        json["music_volume"] = m->getSavedValue<double>("render_music_volume");

        m->setSavedValue(id, json);
        update();
    }
};