#include "../includes.hpp"
#include "../ui/game_ui.hpp"
#include "../utils/subprocess.hpp"

#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/utils/web.hpp>

#include <sstream>
#include <filesystem>
#include <fstream>

#ifdef GEODE_IS_WINDOWS

class $modify(PlayLayer) {
    void showCompleteText() {
        PlayLayer::showCompleteText();
        if (Global::get().renderer.recording && m_levelEndAnimationStarted && Mod::get()->getSavedValue<bool>("render_hide_levelcomplete")) {
            for (CCNode* node : CCArrayExt<CCNode*>(getChildren())) {
                CCSprite* spr = typeinfo_cast<CCSprite*>(node);
                if (!spr) continue;
                if (!isSpriteFrameName(spr, "GJ_levelComplete_001.png")) continue;
                spr->setVisible(false);
            }
        }
    }
};

class $modify(EndLevelLayer) {
    
    void customSetup() {
        EndLevelLayer::customSetup();

        if (!PlayLayer::get()) return;
        if (Global::get().renderer.recording && PlayLayer::get()->m_levelEndAnimationStarted && Mod::get()->getSavedValue<bool>("render_hide_endscreen")) {
            Loader::get()->queueInMainThread([this] {
                setVisible(false);
            });
        }
    }
};

class $modify(FMODAudioEngine) {

    void playEffect(gd::string path, float speed, float p2, float volume) {
        if (path != "playSound_01.ogg" || !Global::get().renderer.recordingAudio)
            FMODAudioEngine::playEffect(path, speed, p2, volume);
    }

};

class $modify(GJBaseGameLayer) {
    void update(float dt) {
        GJBaseGameLayer::update(dt);
        auto& g = Global::get();

        int frame = Global::getCurrentFrame();
        PlayLayer* pl = PlayLayer::get();

        if ((!g.renderer.recording && !g.renderer.recordingAudio) || !pl) return;

        if (g.renderer.recording && frame % static_cast<int>(240.f / g.renderer.fps) == 0)
            return g.renderer.handleRecording(pl, frame);

        if (g.renderer.recordingAudio && !g.renderer.startedAudio) {
            return g.renderer.startAudio(pl);
        }

        if (g.renderer.recordingAudio && frame % static_cast<int>(240.f / g.renderer.fps) == 0)
            return g.renderer.handleAudioRecording(pl, frame);
    }
};

float leftOver = 0.f;

class $modify(CCScheduler) {

    void update(float dt) {
        Renderer& r = Global::get().renderer;
        if (!r.recording) return CCScheduler::update(Global::get().lockDelta ? 1.f / 240.f : dt);

        r.changeRes(false);

        using namespace std::literals;
        
        float newDt = 1.f / 240.f;

        auto startTime = std::chrono::high_resolution_clock::now();
        int mult = static_cast<int>((dt + leftOver) / newDt);

        for (int i = 0; i < mult; ++i) {
            CCScheduler::update(newDt);
            if (std::chrono::high_resolution_clock::now() - startTime > 33.333ms) {
                mult = i + 1;
                break;
            }
        }
        
        leftOver += (dt - newDt * mult);

        r.changeRes(true);
    }

};

#endif

bool Renderer::toggle() {
    auto& g = Global::get();
    if (Loader::get()->isModLoaded("syzzi.click_between_frames")) {
        FLAlertLayer::create("Render", "Disable CBF in Geode to render a level.", "OK")->show();
        return false;
    }

    std::filesystem::path ffmpegPath = g.mod->getSettingValue<std::filesystem::path>("ffmpeg_path");

    if (g.renderer.recording || g.renderer.recordingAudio) {
        g.renderer.recordingAudio ? g.renderer.stopAudio() : g.renderer.stop(Global::getCurrentFrame());
    }
    else if (std::filesystem::exists(ffmpegPath) && ffmpegPath.filename().string() == "ffmpeg.exe") {
        if (!PlayLayer::get()) {
            FLAlertLayer::create("Warning", "<cl>Open a level</c> to start rendering it.", "Ok")->show();
            return false;
        }

        g.renderer.ffmpegPath = ffmpegPath.string();
        std::filesystem::path path = Mod::get()->getSettingValue<std::filesystem::path>("render_folder");

        if (std::filesystem::exists(path))
            g.renderer.start();
        else {
            if (std::filesystem::create_directory(path))
                g.renderer.start();
            else {
                FLAlertLayer::create("Error", "There was an error getting the renders folder. ID: 11", "Ok")->show();
                return false;
            }
        }

    }
    else {
        geode::createQuickPopup(
            "Error",
            "<cl>ffmpeg.exe</c> not found, set the path to it in mod settings.\nOpen download link?",
            "Cancel", "Yes",
            [](auto, bool btn2) {
                if (btn2) {
                    FLAlertLayer::create("Info", "Unzip the downloaded file and look for <cl>ffmpeg.exe</c> in the 'bin' folder.", "Ok")->show();
				    utils::web::openLinkInBrowser("https://www.gyan.dev/ffmpeg/builds/ffmpeg-git-essentials.7z");
                }
            }
        );
        return false;
    }

    Interface::updateLabels();

    return true;
}

void Renderer::start() {
#ifdef GEODE_IS_WINDOWS

    PlayLayer* pl = PlayLayer::get();
    GameManager* gm = GameManager::sharedState();
    Mod* mod = Mod::get();
    fmod = FMODAudioEngine::sharedEngine();

    fps = std::stoi(mod->getSavedValue<std::string>("render_fps"));
    codec = mod->getSavedValue<std::string>("render_codec");
    bitrate = mod->getSavedValue<std::string>("render_bitrate") + "M";
    extraArgs = mod->getSavedValue<std::string>("render_args");
    videoArgs = mod->getSavedValue<std::string>("render_video_args");
    extraAudioArgs = mod->getSavedValue<std::string>("render_audio_args");
    SFXVolume = mod->getSavedValue<double>("render_sfx_volume");
    musicVolume = mod->getSavedValue<double>("render_music_volume");
    stopAfter = geode::utils::numFromString<float>(mod->getSavedValue<std::string>("render_seconds_after")).unwrapOr(0.f);
    audioMode = AudioMode::Off;
    std::string extension = mod->getSavedValue<std::string>("render_file_extension");

    if (mod->getSavedValue<bool>("render_only_song")) audioMode = AudioMode::Song;
    if (mod->getSavedValue<bool>("render_record_audio")) audioMode = AudioMode::Record;

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::string filename = fmt::format("render_{}_{}{}", std::string_view(pl->m_level->m_levelName), std::to_string(timestamp), extension);
    std::string path = (Mod::get()->getSettingValue<std::filesystem::path>("render_folder") / filename).string();

    width = std::stoi(mod->getSavedValue<std::string>("render_width2"));
    height = std::stoi(mod->getSavedValue<std::string>("render_height"));

    if (width % 2 != 0)
        width++;
    if (height % 2 != 0)
        height++;

    renderer.width = width;
    renderer.height = height;
    ogRes = cocos2d::CCEGLView::get()->getDesignResolutionSize();
    ogScaleX = cocos2d::CCEGLView::get()->m_fScaleX;
    ogScaleY = cocos2d::CCEGLView::get()->m_fScaleY;
    changeRes(false);

    dontRender = true;
    recording = true;
    frameHasData = false;
    levelFinished = false;
    startedAudio = false;
    timeAfter = 0.f;
    finishFrame = 0;
    pauseAttempts = 0;
    lastFrame_t = extra_t = 0;

    if (!pl->m_levelEndAnimationStarted && pl->m_isPaused)
        Global::get().restart = true;

    if (Global::get().state != state::playing)
        Macro::togglePlaying();

    auto songFile = pl->m_level->getAudioFileName();
    if (pl->m_level->m_songID == 0)
        songFile = cocos2d::CCFileUtils::sharedFileUtils()->fullPathForFilename(songFile.c_str(), false);

    float songOffset = pl->m_levelSettings->m_songOffset + (fmod->m_musicOffset / 1000.f) + (levelStartFrame / 240.f);
    bool fadeIn = pl->m_levelSettings->m_fadeIn;
    bool fadeOut = pl->m_levelSettings->m_fadeOut;

    currentFrame.resize(width * height * 3, 0);
    renderedFrames.clear();

    renderer.begin();


    std::thread([&, path, songFile, songOffset, fadeIn, fadeOut, extension]() {

        if (!codec.empty()) codec = "-c:v " + codec + " ";
        if (!bitrate.empty()) bitrate = "-b:v " + bitrate + " ";
        if (extraArgs.empty()) extraArgs = "-pix_fmt yuv420p";
        if (videoArgs.empty()) videoArgs = "colorspace=all=bt709:iall=bt470bg:fast=1";

        float fadeInTime = geode::utils::numFromString<float>(Mod::get()->getSavedValue<std::string>("render_fade_in_time")).unwrapOr(0.f);
        bool fadeInVideo = Mod::get()->getSavedValue<bool>("render_fade_in") && fadeInTime != 0.f;
        float fadeOutTime = geode::utils::numFromString<float>(Mod::get()->getSavedValue<std::string>("render_fade_out_time")).unwrapOr(0.f);
        bool fadeOutVideo = Mod::get()->getSavedValue<bool>("render_fade_out") && fadeOutTime != 0.f;

        std::string fadeArgs;
        if (fadeInVideo)
            fadeArgs = fmt::format(",fade=t=in:st=0:d={}", fadeInTime);

        std::string command = fmt::format(
            "\"{}\" -y -f rawvideo -pix_fmt rgb24 -s {}x{} -r {} -i - {}{}{} -vf \"vflip,{}{}\" -an \"{}\" ",
            ffmpegPath,
            std::to_string(width),
            std::to_string(height),
            std::to_string(fps),
            codec,
            bitrate,
            extraArgs,
            videoArgs,
            fadeArgs,
            path
        );

        log::info("Executing: {}", command);

        auto process = subprocess::Popen(command);
        while (recording || pause || recordingAudio || frameHasData) {
            lock.lock();
            if (frameHasData) {
                const std::vector<uint8_t> frame = currentFrame;
                frameHasData = false;
                lock.unlock();
                process.m_stdin.write(frame.data(), frame.size());
            }
            else lock.unlock();
        }

        if (process.close()) {
            Loader::get()->queueInMainThread([] {
                FLAlertLayer::create("Error", "There was an error saving the render. Wrong render args.", "Ok")->show();
            });
            return;
        }

        Loader::get()->queueInMainThread([] {
            Notification::create("Saving Render...", NotificationIcon::Loading)->show();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        double totalTime = lastFrame_t;
        if (fadeOutTime > totalTime) fadeOutTime = totalTime / 2;
        float fadeOutStart = totalTime - fadeOutTime;

        if (fadeOutVideo) {
            command = fmt::format("\"{}\" -i \"{}\" -vf \"fade=t=out:st={}:d={}\" {}-c:a copy \"{}\"", ffmpegPath, path, fadeOutStart, std::to_string(fadeOutTime), codec, path + "_temp" + extension);


            log::info("Executing (Fade Out): {}", command);
            process = subprocess::Popen(command);
            if (!process.close()) {
                std::filesystem::remove(path);
                std::filesystem::rename(path + "_temp" + extension, path);
            } else log::debug("Fade Out Error xD");
        }

        if ((SFXVolume == 0.f && musicVolume == 0.f) || audioMode == AudioMode::Off || (audioMode == AudioMode::Song && !std::filesystem::exists(songFile)) || (audioMode == AudioMode::Record && !std::filesystem::exists("fmodoutput.wav"))) {
            if (audioMode != AudioMode::Off) {
                Loader::get()->queueInMainThread([] {
                    FLAlertLayer::create("Error", "Song File not found.", "Ok")->show();
                });

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            Loader::get()->queueInMainThread([] {
                Notification::create("Render Saved Without Audio", NotificationIcon::Success)->show();
                if (!Mod::get()->getSavedValue<bool>("render_hide_endscreen")) return;
                if (PlayLayer* pl = PlayLayer::get())
                    if (EndLevelLayer* layer = pl->getChildByType<EndLevelLayer>(0))
                        layer->setVisible(true);
            });

            return;
        }

        wchar_t buffer[MAX_PATH];
        if (!GetTempFileNameW(Utils::widen(std::filesystem::temp_directory_path().string()).c_str(), L"rec", 0, buffer)) {
            Loader::get()->queueInMainThread([] {
                FLAlertLayer::create("Error", "There was an error adding the song. ID: 13", "Ok")->show();
            });
            return;
        }

        std::string tempPath = Utils::narrow(buffer) + "." + std::filesystem::path(path).filename().string();
        std::filesystem::rename(buffer, tempPath);

        std::string tempPathAudio = (Mod::get()->getSaveDir() / "temp_audio_file.wav").string();

        // std::string tempPathAudio = tempPath;

        if (audioMode == AudioMode::Record) {
            command = fmt::format("\"{}\" -i \"fmodoutput.wav\" -acodec pcm_s16le -ar 44100 -ac 2 \"{}\"",
                ffmpegPath, tempPathAudio
            );

           process = subprocess::Popen(command);  // Fix ffmpeg not reading it
            if (process.close()) {
                Loader::get()->queueInMainThread([] {
                    FLAlertLayer::create("Error", "There was an error adding the song. ID: 140", "Ok")->show();
                });
                return;
            }
        }

        {

            std::string fadeInString;
            if ((fadeIn && audioMode == AudioMode::Song) || fadeInVideo) 
                fadeInString = fmt::format(", afade=t=in:d={}", fadeInVideo ? std::to_string(fadeInTime) : "2");

            std::string fadeOutString;
            if ((fadeOut && audioMode == AudioMode::Song) || fadeOutVideo) 
                fadeOutString = fmt::format(
                    ", afade=t=out:d={}:st={}", 
                    fadeOutVideo ? std::to_string(fadeOutTime) : "2",
                    fadeOutVideo ? fadeOutStart : totalTime - timeAfter - 3.5f
                );

            std::string file = audioMode == AudioMode::Song ? songFile : tempPathAudio;
            float offset = audioMode == AudioMode::Song ? songOffset : (isPlatformer ? 0.28f : 0.f);

            if (!extraAudioArgs.empty()) extraAudioArgs += " ";

            command = fmt::format(
                "\"{}\" -y -ss {} -i \"{}\" -i \"{}\" -t {} -c:v copy {} -filter:a \"[1:a]adelay=0|0{}{}\" \"{}\"",
                ffmpegPath,
                offset,
                file,
                path,
                totalTime,
                extraAudioArgs,
                fadeInString,
                fadeOutString,
                tempPath
            );

            log::info("Executing (Audio): {}", command);

            auto process = subprocess::Popen(command);
            if (process.close()) {
                Loader::get()->queueInMainThread([] {
                    FLAlertLayer::create("Error", "There was an error adding the song. Wrong Audio Args.", "Ok")->show();
                });
                return;
            }

        }

        std::filesystem::remove(Utils::widen(path));
        std::filesystem::rename(tempPath, Utils::widen(path));
        std::filesystem::remove(tempPathAudio);
        std::filesystem::remove("fmodoutput.wav");

        Loader::get()->queueInMainThread([] {
            Notification::create("Render Saved With Audio", NotificationIcon::Success)->show();
        });
        
        }).detach();

#endif
}

void Renderer::stop(int frame) {
#ifdef GEODE_IS_WINDOWS
    pause = true;
    recording = false;
    timeAfter = 0.f;
    renderedFrames.clear();
    finishFrame = frame;

    if (PlayLayer* pl = PlayLayer::get()) {
        if (pl->m_hasCompletedLevel) {
            finishFrame = 0;
            levelFinished = true;
        }

        if (pl->m_isPaused && audioMode == AudioMode::Record) {
            if (PauseLayer* layer = Global::getPauseLayer()) {
                layer->onResume(nullptr);
                CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
                if (RecordLayer* xdbot = scene->getChildByType<RecordLayer>(0))
                    xdbot->onClose(nullptr);
            }
        }
    }

    if (audioMode == AudioMode::Record) {
        recordingAudio = true;
        dontRecordAudio = true;
        Notification::create("Recording Audio...", NotificationIcon::Loading)->show();
    }

    pause = false;
    changeRes(true);

#endif
}

void Renderer::changeRes(bool og) {
    #ifdef GEODE_IS_WINDOWS
    cocos2d::CCEGLView* view = cocos2d::CCEGLView::get();
    cocos2d::CCSize res = {0, 0};
    float scaleX = 1.f;
    float scaleY = 1.f;

    res = og ? ogRes : CCSize(320.f * (width / static_cast<float>(height)), 320.f);
    scaleX = og ? ogScaleX : (width / res.width);
    scaleY = og ? ogScaleY : (height / res.height);

    if (res == CCSize(0, 0) && !og) return changeRes(true);
       
    CCDirector::sharedDirector()->m_obWinSizeInPoints = res;
    view->setDesignResolutionSize(res.width, res.height, ResolutionPolicy::kResolutionExactFit);
    view->m_fScaleX = scaleX;
    view->m_fScaleY = scaleY;
    #endif
}

void MyRenderTexture::begin() {
#ifdef GEODE_IS_WINDOWS

    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);

    texture = new CCTexture2D();
    {
        auto data = malloc(width * height * 3);
        memset(data, 0, width * height * 3);
        texture->initWithData(data, kCCTexture2DPixelFormat_RGB888, width, height, CCSize(static_cast<float>(width), static_cast<float>(height)));
        free(data);
    }

    glGetIntegerv(GL_RENDERBUFFER_BINDING_EXT, &old_rbo);

    glGenFramebuffersEXT(1, &fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture->getName(), 0);

    texture->setAliasTexParameters();

    texture->autorelease();

    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, old_rbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fbo);

#endif
}

void MyRenderTexture::capture(std::mutex& lock, std::vector<uint8_t>& data, volatile bool& lul) {
#ifdef GEODE_IS_WINDOWS
    glViewport(-1, 1.0, width, height);

    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &old_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

    CCDirector* director = CCDirector::sharedDirector();
    PlayLayer::get()->visit();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    lock.lock();
    lul = true;
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    lock.unlock();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, old_fbo);
    director->setViewport();
#endif
}

void Renderer::captureFrame() {
#ifdef GEODE_IS_WINDOWS

    while (frameHasData) {}
    renderer.capture(lock, currentFrame, frameHasData);

#endif
}

void Renderer::handleRecording(PlayLayer* pl, int frame) {
#ifdef GEODE_IS_WINDOWS

    if (!pl) stop();
    isPlatformer = pl->m_levelSettings->m_platformerMode;
    if (dontRender || pl->m_player1->m_isDead) return;

    auto& g = Global::get();
    if (renderedFrames.contains(frame) && frame > 10)
        return;

    renderedFrames.insert(frame);

    if (!pl->m_hasCompletedLevel || timeAfter < stopAfter) {

        float dt = 1.f / static_cast<double>(fps);
        if (pl->m_hasCompletedLevel) {
            timeAfter += dt;
            levelFinished = true;
        }

        float time = pl->m_gameState.m_levelTime + extra_t - lastFrame_t;
        if (time >= dt) {
            extra_t = time - dt;
            lastFrame_t = pl->m_gameState.m_levelTime;

            int correctMusicTime = static_cast<int>((frame / 240.f + pl->m_levelSettings->m_songOffset) * 1000);
            correctMusicTime += fmod->m_musicOffset; // Global music offset

            if (fmod->getMusicTimeMS(0) - correctMusicTime >= 110)
                fmod->setMusicTimeMS(correctMusicTime, true, 0);

            captureFrame();
        }
    }
    else stop(frame);

#endif
}

bool Renderer::tryPause() {
    if (!recordingAudio) return true;

    pauseAttempts++;

    if (pauseAttempts > 5) {
        pauseAttempts = 0;
        stopAudio();
        return true;
    }

    return false;
}

void Renderer::startAudio(PlayLayer* pl) {
    EndLevelLayer* endLevelLayer = pl->getChildByType<EndLevelLayer>(0);
    if (dontRecordAudio) return;

    if (pl->m_levelEndAnimationStarted && endLevelLayer != nullptr) {
        CCKeyboardDispatcher::get()->dispatchKeyboardMSG(enumKeyCodes::KEY_Space, true, false);
        CCKeyboardDispatcher::get()->dispatchKeyboardMSG(enumKeyCodes::KEY_Space, false, false);
    }
    else if (!pl->m_levelEndAnimationStarted) {

        if (!pl->m_isPaused)
            pl->pauseGame(false);

        if (Global::get().state != state::playing)
            Macro::togglePlaying();

        Global::get().restart = true;

        if (PauseLayer* layer = Global::getPauseLayer())
            layer->onResume(nullptr);

        auto fmod = FMODAudioEngine::sharedEngine();
        fmod->m_globalChannel->getVolume(&ogSFXVol);
        fmod->m_backgroundMusicChannel->getVolume(&ogMusicVol);

        FMODAudioEngine::sharedEngine()->m_system->setOutput(FMOD_OUTPUTTYPE_WAVWRITER);
        startedAudio = true;
        pauseAttempts = 0;
        if (CCNode* lbl = pl->getChildByID("recording-audio-label"_spr))
            lbl->setVisible(true);
    }
}

void Renderer::stopAudio() {
    FMODAudioEngine::sharedEngine()->m_system->setOutput(FMOD_OUTPUTTYPE_AUTODETECT);
    auto fmod = FMODAudioEngine::sharedEngine();
	fmod->m_globalChannel->setVolume(ogSFXVol);
	fmod->m_backgroundMusicChannel->setVolume(ogMusicVol);

    recordingAudio = false;
    if (PlayLayer* pl = PlayLayer::get())
        if (CCNode* lbl = pl->getChildByID("recording-audio-label"_spr))
                lbl->setVisible(false);
}

void Renderer::handleAudioRecording(PlayLayer* pl, int frame) {
#ifdef GEODE_IS_WINDOWS
    auto& g = Global::get();
    
	fmod->m_globalChannel->setVolume(SFXVolume);
	fmod->m_backgroundMusicChannel->setVolume(musicVolume);

    if (!pl) {
        g.renderer.stopAudio();
        return;
    }

    if ((finishFrame != 0 && frame >= finishFrame) || pl->m_player1->m_isDead) {
        g.renderer.stopAudio();
        return;
    }

    log::debug("{} {}", timeAfter, stopAfter);

    if (!pl->m_hasCompletedLevel || timeAfter < stopAfter) {
        float dt = 1.f / static_cast<double>(fps);
        if (pl->m_hasCompletedLevel)
            timeAfter += dt;
    }
    else
        g.renderer.stopAudio();

#endif
}