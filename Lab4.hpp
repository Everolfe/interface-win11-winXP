#pragma once
#include "LabWindow.hpp"
#include "WebcamCapture.hpp"
#include "Button.hpp"
#include "Character.hpp"
//#include <SFML/Graphics.hpp>
//#include <string>
//#include <vector>
//#include <atomic>
//#include <thread>
//#include <chrono>
//#include <sstream>
//#include <iomanip>
//#include <fstream>
//
//#include <windows.h>
//#include <thread>
//#include <atomic>
//#include <iostream>
//#include <sstream>
//#include <iomanip>
//#include <fstream>
//#include <opencv2/opencv.hpp>

//#include "Source.hpp"
class Lab4 : public LabWindow {
private:
    sf::RenderWindow& window;
    sf::Font& font;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Vector2f currentWindowSize;

    Character character;

    // Webcam capture
    WebcamCapture camera;
    sf::Texture cameraTexture;
    sf::Sprite cameraSprite;
    sf::Image currentCameraImage;
    bool cameraInitialized = false;

    // Camera information
    std::vector<std::string> cameraInfoLines;
    std::vector<CameraInfo> availableCameras;
    int selectedCameraIndex = 0;

    // Recording state
    std::atomic<bool> isRecording{ false };
    std::vector<sf::Image> videoFrames;
    std::atomic<bool> stealthMode{ false };
    std::chrono::time_point<std::chrono::steady_clock> recordingStartTime;
    float maxRecordingDuration = 10.f; // максимальная длительность видео в секундах

    // Window state
    std::atomic<bool> windowHidden{ false };

    // Buttons
    Button hideWindowButton;

    // Mouse interaction
    sf::Vector2f currentMousePos;
    bool mouseOverHideWindowButton = false;
    bool showCameraPreview = false;
    bool cameraActive = false;

    void enumerateCameras();
    std::string getCurrentTimestamp();
    void initializeCamera();
    void capturePhoto(bool stealth = false);
    void startStopVideo(bool stealth = false);
    void saveVideo();
    void toggleWindowVisibility();
    void registerGlobalHotkeys();
    void unregisterGlobalHotkeys();
    static Lab4* currentInstance; // Для доступа из статических функций

public:
    Lab4(sf::RenderWindow& win, sf::Font& f);

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