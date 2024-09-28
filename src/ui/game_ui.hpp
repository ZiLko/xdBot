
#pragma once

#include "../includes.hpp"
#include "record_layer.hpp"
#include "button_edit_layer.hpp"

class Interface {

public:

    void onSpeedhack() {
        Global::toggleSpeedhack();
    }

    void onFrameStepper() {
        if (!Global::get().frameStepper)
            Global::toggleFrameStepper();
        else
            Global::frameStep();
    }

    void onFrameStepperOff() {
        Global::frameStepperOff();
    }

    static void openButtonEditor() {
        ButtonEditLayer* layer = ButtonEditLayer::create();
        layer->m_noElasticity = true;
        layer->show();
    }

    static void addLabels(PlayLayer* pl);

    static void addButtons(PlayLayer* pl);

    static void updateLabels();

    static void updateButtons();

};