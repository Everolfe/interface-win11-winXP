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
struct PCIDevice {
    std::string bus;
    std::string device;
    std::string function;
    std::string vendorID;
    std::string deviceID;
    std::string vendorName;
    std::string deviceName;
};

class Lab2 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;

    Character character;

    std::vector<PCIDevice> pciDevices;
    std::vector<std::string> pciInfoLines;

    sf::Vector2f currentMousePos;
    bool mouseOverBackButton = false;
    bool mouseOverPrevButton = false;
    bool mouseOverNextButton = false;

    Button prevButton;
    Button nextButton;

    // Page navigation variables
    int currentPage = 0;
    int devicesPerPage = 10;

    void loadPCIData();
    void parsePCIDevices();
    std::string getVendorName(const std::string& vendorID);
    std::string getDeviceName(const std::string& vendorID, const std::string& deviceID);
    void updateDeviceDisplay();

public:
    Lab2(sf::RenderWindow& win, sf::Font& f)
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