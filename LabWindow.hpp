#pragma once
//#include <SFML/Graphics.hpp>
//#include<Windows.h>
#include"AdditionalFunc.hpp"


//#include "Source.hpp"
class LabWindow {
public:
    static LabWindow* activeWindow; 
    virtual ~LabWindow() = default;
    virtual void run() = 0;
    virtual void initializeButtons() = 0;
    virtual void handleEvents(bool& running) = 0;
    virtual void render() = 0;
    virtual bool loadBackground() = 0;
    virtual void updateBackgroundSize() = 0;
    virtual void updateLayout() = 0;
    virtual void updateButtonPositions() = 0;
    virtual void updateCharacter(float deltaTime) = 0;
    virtual void refreshDeviceList() {};
};
