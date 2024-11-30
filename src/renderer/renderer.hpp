#include "../includes.hpp"

enum AudioMode {
    Off = 0,
    Song = 1,
    Record = 2
};

class MyRenderTexture {
public:
    unsigned width, height;
    int old_fbo, old_rbo;
    unsigned fbo;
    geode::prelude::CCTexture2D* texture = nullptr;
    void begin();
    void capture(std::mutex& lock, std::vector<uint8_t>& data, volatile bool& lul);
};

class Renderer {
public:

    Renderer() : width(1920), height(1080), fps(60) {}

    volatile bool frameHasData;
    bool levelFinished = false;
    bool recording = false;
    bool pause = false;
    int audioMode = 0;

    bool dontRender = false;
    bool dontRecordAudio = false;
    bool recordingAudio = false;
    bool startedAudio = false;
    bool isPlatformer = false;
    int finishFrame = 0;
    int levelStartFrame = 0;

    float stopAfter = 3.f;
    float timeAfter = 0.f;
    unsigned width, height;
    unsigned fps;
    double lastFrame_t, extra_t;

    MyRenderTexture renderer;
    std::vector<uint8_t> currentFrame;
    std::mutex lock;
    std::string codec = "", bitrate = "12M", extraArgs = "", videoArgs = "", extraAudioArgs = "", path = "";
    std::string ffmpegPath = (geode::dirs::getGameDir() / "ffmpeg.exe").string();
    std::unordered_set<int> renderedFrames;

    FMODAudioEngine* fmod = nullptr;
    cocos2d::CCSize ogRes = {0, 0};
    float ogScaleX = 1.f;
    float ogScaleY = 1.f;

    void captureFrame();
    void changeRes(bool og);

    void start();
    void startAudio(PlayLayer* pl);

    void stop(int frame = 0);
    void stopAudio();

    void handleRecording(PlayLayer* pl, int frame);
    void handleAudioRecording(PlayLayer* pl, int frame);
    
    static bool toggle();
};