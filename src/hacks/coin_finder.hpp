#include "../includes.hpp"
#include <Geode/modify/PlayLayer.hpp>

class CoinFinderNode : public cocos2d::CCDrawNode {

public:

    static CoinFinderNode* create() {
        CoinFinderNode* ret = new CoinFinderNode();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

};

class CoinFinder {

public:

    static void finderOff() {
        if (drawNode()) {
            drawNode()->clear();
            drawNode()->setVisible(false);
        }
    }

    static cocos2d::CCDrawNode* drawNode() {
        static CoinFinderNode* instance = nullptr;

        if (!instance) {
            instance = CoinFinderNode::create();
            instance->retain();

            cocos2d::_ccBlendFunc  blendFunc;
            blendFunc.src = GL_SRC_ALPHA;
            blendFunc.dst = GL_ONE_MINUS_SRC_ALPHA;

            instance->setBlendFunc(blendFunc);
        }

        return instance;
    }

};