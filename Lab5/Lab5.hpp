#pragma once
#include "LabWindow.hpp"
#include "Button.hpp"
#include "Character.hpp"
//#include <SFML/Graphics.hpp>
//#include <SetupAPI.h>
//#include <Cfgmgr32.h>
//#include <Dbt.h>
//#include <Usbiodef.h>
//#include <initguid.h>
//
//#include <string>
//#include <vector>
//#include <algorithm>
//#include <sstream>
//#include <mutex>
//#include <chrono>
//
//#include <iostream>
//
//#pragma comment(lib, "setupapi.lib")
//#pragma comment(lib, "cfgmgr32.lib")


//#include "Source.hpp"
// GUID интерфейса USB устройств
DEFINE_GUID(GUID_DEVINTERFACE_USB_DEVICE,
    0xA5DCBF10L, 0x6530, 0x11D2,
    0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);



enum class NotifType { Info, Success, Warning, Error };

struct DeviceInfo {
    std::wstring name;
    std::wstring devicePath;
    DEVINST devInst = 0;
    bool ejectable = false;
    bool requestedEject = false;
    std::string getDeviceKey() const {
        return std::to_string(devInst) + std::string(devicePath.begin(), devicePath.end());
    }
};

struct Notification {
    std::wstring text;
    NotifType type;
    std::chrono::steady_clock::time_point created;
    int lifetime_ms;

    Notification(const std::wstring& t = L"", NotifType ty = NotifType::Info, int life = 3000)
        : text(t), type(ty), created(std::chrono::steady_clock::now()), lifetime_ms(life) {}

    float age_fraction() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - created).count();
        float f = float(elapsed) / float(lifetime_ms);
        return (f < 0) ? 0 : (f > 1 ? 1 : f);
    }

    bool expired() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - created).count();
        return elapsed >= lifetime_ms;
    }
};

class Lab5 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;



    Character character;

    std::vector<DeviceInfo> devices;
    std::vector<DeviceInfo> previousDevices;
    std::vector<Notification> notifications;
    std::mutex devices_mtx;
    std::mutex notifs_mtx;

    Button refreshButton;
    Button ejectButton;
    int selectedDeviceIndex;

    sf::Vector2f currentMousePos;
    bool mouseOverRefreshButton = false;
    bool mouseOverEjectButton = false;

    sf::Clock refreshClock;
    float refreshInterval = 1.0f; 

    void pushNotification(const std::wstring& text, NotifType t = NotifType::Info, int life = 3000);
    bool shouldShowDevice(const std::wstring& desc, bool ejectable);
    void fillDeviceProperties(DeviceInfo& out, SP_DEVICE_INTERFACE_DATA& ifaceData, HDEVINFO deviceInfoSet);
    bool tryEjectByDevInst(DEVINST devInst, const std::wstring& name);
    sf::Color notifColor(NotifType t);
    void drawNotifications(sf::RenderWindow& window);
    void drawDeviceList(sf::RenderWindow& window);
    void detectDeviceChanges(const std::vector<DeviceInfo>& oldDevices, const std::vector<DeviceInfo>& newDevices);
    std::string wstringToUTF8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }
private:

public:
    Lab5(sf::RenderWindow& win, sf::Font& f);
    void refreshDeviceList();
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