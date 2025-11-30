#pragma once
#include "LabWindow.hpp"
#include "Button.hpp"
#include "Character.hpp"
//#include <SFML/Graphics.hpp>
//#include <string>
//#include <iostream>
//#include <powrprof.h>
//#include <winbase.h>
//#include <setupapi.h>
//#include <devguid.h>
//#include <Poclass.h>
//#pragma comment(lib, "PowrProf.lib")
//#pragma comment(lib, "Setupapi.lib")

//#include "Source.hpp"
class Lab1 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;

    Character character;

    std::string batteryTypeStr;
    std::string powerSourceStr = "Unknown";
    std::string batteryStatusStr = "Unknownll";
    std::string powerModeStr = "Normal";

    sf::Vector2f currentMousePos;
    bool mouseOverSleepButton = false;
    bool mouseOverHibernateButton = false;



    int batteryLevelPercent = -1;
    int batteryRunTimeMinutes = -1;
    int batteryFullLifeTimeMinutes = -1;

    bool onBattery = false;
    bool isCharging = false;

    Button sleepButton;
    Button hibernateButton;

public:
    Lab1(sf::RenderWindow& win, sf::Font& f)
        : window(win), font(f),
        character(sf::Vector2f(win.getSize())),
        sleepButton("Sleep Mode", font),
        hibernateButton("Hibernate Mode", font)
    {};
    void run() override;
    void initializeButtons() override;
    void handleEvents(bool& running) override;
    void render() override;


    void updatePowerMode(const SYSTEM_POWER_STATUS& status);
    void updatePowerSource(const SYSTEM_POWER_STATUS& status);
    void updateBatteryLevel(const SYSTEM_POWER_STATUS& status);
    void updateBatteryTimes(const SYSTEM_POWER_STATUS& status);
    void updateBatteryStatus(const SYSTEM_POWER_STATUS& status);
    std::string GetBatteryType();
    void updatePowerInfo();

    bool loadBackground() override;
    void updateBackgroundSize() override;
    void updateLayout() override;
    void updateButtonPositions() override;
    void updateCharacter(float deltaTime) override;
};