#pragma once
#include "LabWindow.hpp"
#include "Button.hpp"
#include "Character.hpp"
//#include <SFML/Graphics.hpp>
//#include <string>
//#include <vector>
//#include <iostream>
//#include <fstream>
//#include <sstream>
//#include <algorithm>
//#include <iomanip>

//#include "Source.hpp"
struct DiskDevice {
    std::string model;
    std::string serial;
    std::string firmware;
    std::string type;
    std::string inter;
    std::string totalSize;
    std::string usedSpace;
    std::string freeSpace;
    std::string supportedModes;
};

class Lab3 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;

    Character character;

    std::vector<DiskDevice> diskDevices;
    std::vector<std::string> diskInfoLines;

    sf::Vector2f currentMousePos;
    bool mouseOverPrevButton = false;
    bool mouseOverNextButton = false;

    Button prevButton;
    Button nextButton;

    // Page navigation variables
    int currentPage = 0;
    int devicesPerPage = 5;

    void loadDiskData();
    void parseDiskDevices();
    void updateDeviceDisplay();

public:
    Lab3(sf::RenderWindow& win, sf::Font& f)
        : window(win), font(f),
        character(sf::Vector2f(win.getSize())),
        prevButton("Previous", font),
        nextButton("Next", font)
    {};

    void run() override;
    void initializeButtons() override;
    void handleEvents(bool& running) override;
    void render() override;

    bool loadBackground() override;
    void updateBackgroundSize() override;
    void updateLayout() override;
    void updateButtonPositions() override;
    void updateCharacter(float deltaTime) override;
};