#pragma once
#include "LabWindow.hpp"
#include "Button.hpp"
#include "Character.hpp"
#include "BluetoothFunc.hpp"

class Lab6 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;

    Character character;

    std::vector<BluetoothDevice> bluetoothDevices;
    std::vector<std::string> bluetoothInfoLines;

    std::mutex devices_mtx;
    std::string selectedFilePath;
    int selectedDeviceIndex = -1;
    bool isServerRunning = false;
    std::string statusMessage;

    Button refreshButton;
    Button serverButton;
    Button chooseFileButton;
    Button sendButton;
    Button nextButton;    // Новая кнопка
    Button previousButton; // Новая кнопка

    sf::Vector2f currentMousePos;
    bool mouseOverRefreshButton = false;
    bool mouseOverServerButton = false;
    bool mouseOverChooseFileButton = false;
    bool mouseOverSendButton = false;
    bool mouseOverNextButton = false;
    bool mouseOverPreviousButton = false;

    // Пагинация
    size_t currentPage = 0;
    size_t devicesPerPage = 11;

    void refreshDeviceList();
    void updateStatus(const std::string& message);
    std::string getFileSizeString(const std::string& filePath);
    void sendFileToDevice();
    void runServer();
    void stopServer();
    void updatePageButtons(); // Обновление состояния кнопок пагинации

public:
    Lab6(sf::RenderWindow& win, sf::Font& f);

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