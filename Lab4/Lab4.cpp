#define _CRT_SECURE_NO_WARNINGS
#include "Lab4.hpp"



#ifdef _WIN32


// Глобальные переменные для управления окнами
HWND hAppWindow = nullptr;
HWND hConsoleWindow = nullptr;
std::atomic<bool> g_windowsHidden{ false };

// Глобальные горячие клавиши
std::atomic<bool> g_shouldShowWindows{ false };
std::atomic<bool> g_shouldCapturePhoto{ false };
std::atomic<bool> g_shouldCaptureStealthPhoto{ false };
std::atomic<bool> g_shouldStartStopVideo{ false };
std::atomic<bool> g_shouldStartStopStealthVideo{ false };

// Указатель на текущий экземпляр для обратных вызовов
Lab4* Lab4::currentInstance = nullptr;

void toggleWindowsVisibility(bool hide) {
    if (hide) {
        if (hAppWindow) {
            ShowWindow(hAppWindow, SW_HIDE);
        }
        g_windowsHidden = true;
        std::cout << "Windows hidden. Press F1 to show windows." << std::endl;
    }
    else {
        if (hAppWindow) ShowWindow(hAppWindow, SW_SHOW);
        if (hConsoleWindow) ShowWindow(hConsoleWindow, SW_SHOW);
        g_windowsHidden = false;
        std::cout << "Windows shown." << std::endl;
    }
}

void checkGlobalHotkeys() {
    bool f1Pressed = false;
    bool f2Pressed = false;
    bool f3Pressed = false;

    while (true) {
        // F1 - показать окна
        if (GetAsyncKeyState(VK_F1) & 0x8000) {
            if (!f1Pressed) {
                f1Pressed = true;
                if (g_windowsHidden) {
                    toggleWindowsVisibility(false);
                }
            }
        }
        else {
            f1Pressed = false;
        }

        // F2 - сделать фото (даже когда окна скрыты)
        if (GetAsyncKeyState(VK_F2) & 0x8000) {
            if (!f2Pressed) {
                f2Pressed = true;
                g_shouldCapturePhoto = true;
            }
        }
        else {
            f2Pressed = false;
        }

        // F3 - начать/остановить видео (даже когда окна скрыты)
        if (GetAsyncKeyState(VK_F3) & 0x8000) {
            if (!f3Pressed) {
                f3Pressed = true;
                g_shouldStartStopVideo = true;
            }
        }
        else {
            f3Pressed = false;
        }

        Sleep(50); // Проверяем каждые 50мс
    }
}
#endif

Lab4::Lab4(sf::RenderWindow& win, sf::Font& f)
    : window(win), font(f),
    character(sf::Vector2f(win.getSize())),
    hideWindowButton("Hide Windows", font)
{
#ifdef _WIN32
    hAppWindow = window.getSystemHandle();
    hConsoleWindow = GetConsoleWindow();
    Lab4::currentInstance = this;

    std::thread hotkeyThread(checkGlobalHotkeys);
    hotkeyThread.detach();
#endif

    loadBackground();
    enumerateCameras();
}

void Lab4::enumerateCameras() {
    availableCameras = camera.listCameras();
    cameraInfoLines.clear();

    if (availableCameras.empty()) {
        cameraInfoLines.push_back("No cameras found");
    }
    else {
        cameraInfoLines.push_back("Available Cameras:");
        for (size_t i = 0; i < availableCameras.size(); i++) {
            std::wstring ws = availableCameras[i].name;
            std::string name(ws.begin(), ws.end());
            cameraInfoLines.push_back("[" + std::to_string(i) + "] " + name);
        }
    }
}

std::string Lab4::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
    ss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Lab4::initializeCamera() {
    if (!cameraInitialized) {
        cameraInitialized = camera.initialize(selectedCameraIndex);
        if (cameraInitialized) {
            std::cout << "Camera initialized successfully\n";
        }
        else {
            std::cout << "Failed to initialize camera\n";
        }
    }
}

void Lab4::capturePhoto(bool stealth) {
#ifdef _WIN32
    bool wasHidden = g_windowsHidden;
    if (stealth && !wasHidden) {
        toggleWindowsVisibility(true);
    }
#endif

    cameraActive = true;
    showCameraPreview = true;
    initializeCamera();

    if (!cameraInitialized) {
#ifdef _WIN32
        if (stealth && !wasHidden) toggleWindowsVisibility(false);
#endif
        return;
    }

    Sleep(1000);

    if (camera.getFrame(currentCameraImage)) {
        std::string prefix = stealth ? "photo_" : "photo_";
        std::string filename = prefix + getCurrentTimestamp() + ".png";
        std::cout << "Photo saved: " << filename << "\n";
        currentCameraImage.saveToFile(filename);
    }

    camera.cleanup();
    cameraInitialized = false;
    cameraActive = false;
    showCameraPreview = false;

#ifdef _WIN32

#endif
}

void Lab4::startStopVideo(bool stealth) {
    if (!isRecording) {
#ifdef _WIN32
        bool wasHidden = g_windowsHidden;
        if (stealth && !wasHidden) {
            toggleWindowsVisibility(true);
        }
#endif

        cameraActive = true;
        showCameraPreview = true;
        initializeCamera();
        if (!cameraInitialized) {
#ifdef _WIN32
            if (stealth && !wasHidden) toggleWindowsVisibility(false);
#endif
            return;
        }

        videoFrames.clear();
        isRecording = true;
        stealthMode = stealth;

        if (stealth) {
            recordingStartTime = std::chrono::steady_clock::now();
        }

        std::cout << "Recording started" << std::endl;
    }
    else {
        isRecording = false;
        saveVideo();
        camera.cleanup();
        cameraInitialized = false;
        cameraActive = false;
        showCameraPreview = false;

#ifdef _WIN32
#endif

        std::cout << "Recording stopped" << std::endl;
    }
}

void Lab4::toggleWindowVisibility() {
#ifdef _WIN32
    toggleWindowsVisibility(!g_windowsHidden);
    if (g_windowsHidden) {
        hideWindowButton.setText("Hide Windows");
    }
    else {
        hideWindowButton.setText("Hide Windows");
    }
#endif
}

void Lab4::saveVideo() {
    if (videoFrames.empty()) return;
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);

    std::string prefix = stealthMode ? "video_" : "video_";
    std::string filename = prefix + getCurrentTimestamp() + ".avi";

    int width = camera.getWidth();
    int height = camera.getHeight();
    int fps = 30;

    cv::VideoWriter writer(filename,
        cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
        fps,
        cv::Size(width, height));

    if (!writer.isOpened()) {
        std::cout << "Error: cannot open video file for writing\n";
        return;
    }

    for (auto& frame : videoFrames) {
        const sf::Uint8* pixels = frame.getPixelsPtr();
        cv::Mat img(height, width, CV_8UC4, (void*)pixels);
        cv::Mat bgr;
        cv::cvtColor(img, bgr, cv::COLOR_RGBA2BGR);
        writer.write(bgr);
    }

    writer.release();
    std::cout << "Video saved: " << filename << "\n";

    videoFrames.clear();
    stealthMode = false;
}

void Lab4::run() {
    initializeButtons();

    bool running = true;
    sf::Clock clock;

    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents(running);
        updateCharacter(deltaTime);

        // Проверяем глобальные горячие клавиши
#ifdef _WIN32
        if (g_shouldShowWindows && g_windowsHidden) {
            toggleWindowsVisibility(false);
            g_shouldShowWindows = false;
        }

        if (g_shouldCapturePhoto) {
            capturePhoto(false);
            g_shouldCapturePhoto = false;
        }

        if (g_shouldStartStopVideo) {
            startStopVideo(false);
            g_shouldStartStopVideo = false;
        }
#endif

        // Update camera frame if initialized
        if (cameraActive && cameraInitialized && camera.getFrame(currentCameraImage)) {
            cameraTexture.loadFromImage(currentCameraImage);
            cameraSprite.setTexture(cameraTexture, true);

            float scale = 0.4f;
            cameraSprite.setScale(scale, scale);
            cameraSprite.setPosition(20, 250);

            if (isRecording && videoFrames.size() < 3000) {
                videoFrames.push_back(currentCameraImage);
            }
        }

        // Автоматическая остановка stealth-видео
        if (isRecording && stealthMode) {
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - recordingStartTime).count();
            if (elapsed >= maxRecordingDuration) {
                std::cout << "Stealth video recording stopped automatically (time limit reached)" << std::endl;
                startStopVideo(true);
            }
        }

        render();

        Sleep(10);
    }

    if (isRecording) {
        isRecording = false;
        saveVideo();
    }

    camera.cleanup();

#ifdef _WIN32
    if (g_windowsHidden) {
        toggleWindowsVisibility(false);
    }
#endif
}

void Lab4::initializeButtons() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();

    hideWindowButton.setOnClick([this]() {
        toggleWindowVisibility();
    });

    if (!character.loadTextures("character_lab1_normal.png", "character_lab1_normal.png", "walk.png")) {
        std::cout << "Using default character texture" << std::endl;
    }

    character.setInitialScale(0.7f);
    character.setBaseAnimationSpeed(0.1f);
    character.setPosition(currentWindowSize.x * 0.1f, currentWindowSize.y * 0.7f);
    character.setRandomMovement();
}

void Lab4::handleEvents(bool& running) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
        }
        else if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
            case sf::Keyboard::Escape:
                running = false;
                break;
            case sf::Keyboard::H: 
                toggleWindowVisibility();
                break;
            }
        }
        else if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
            window.setView(sf::View(visibleArea));
            updateLayout();
        }

        hideWindowButton.handleEvent(event, window);

        if (event.type == sf::Event::MouseMoved) {
            currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            mouseOverHideWindowButton = hideWindowButton.isMouseOver(currentMousePos);

            if (mouseOverHideWindowButton) {
                character.setButtonTargetLeft(hideWindowButton.getPosition(), hideWindowButton.getSize());
                character.setPointing(true);
            }
            else {
                character.setPointing(false);
                character.moveToMouse(currentMousePos);
            }
        }
    }
}
void Lab4::render() {
    window.clear(sf::Color(30, 30, 40));

    if (backgroundTexture.getSize().x > 0)
        window.draw(backgroundSprite);

    sf::RectangleShape infoPanel(sf::Vector2f(currentWindowSize.x * 0.5f, currentWindowSize.y * 0.4f));
    infoPanel.setPosition(currentWindowSize.x * 0.02f, currentWindowSize.y * 0.02f);
    infoPanel.setFillColor(sf::Color(0, 0, 0, 180));
    infoPanel.setOutlineColor(sf::Color::White);
    infoPanel.setOutlineThickness(1.f);
    window.draw(infoPanel);

    std::stringstream info;
    info << "Camera Controls:\n"
        << "F2 - Take Photo\n"
        << "F3 - Start/Stop Video\n"
        << "H - Hide window\n"
        << "F1 - Show Window\n";

    info << "Camera Info:\n";
    if (!availableCameras.empty() && selectedCameraIndex < availableCameras.size()) {
        std::wstring ws = availableCameras[selectedCameraIndex].name;
        std::string name(ws.begin(), ws.end());
        info << "Name: " << name << " | Index: " << selectedCameraIndex << "\n"
            << "Resolution: " << camera.getWidth() << "x" << camera.getHeight() << "\n"
            << "Resolutions: ";

        // Компактный вывод разрешений
        int count = 0;
        for (const auto& res : availableCameras[selectedCameraIndex].resolutions) {
            if (count < 5) {
                info << res.first << "x" << res.second;
                if (count < 5 && availableCameras[selectedCameraIndex].resolutions.size() > 1) {
                    info << ", ";
                }
                count++;
            }
            else if (count == 5) {
                count = 0;
                break;
            }
        }

        info << "\nFPS: ";
        count = 0;
        for (double fps : availableCameras[selectedCameraIndex].fpsOptions) {
            if (count < 1) {
                info << std::fixed << std::setprecision(1) << fps;
                if (count < 1 && availableCameras[selectedCameraIndex].fpsOptions.size() > 1) {
                    info << ", ";
                }
                count++;
            }
            else if (count == 2 && availableCameras[selectedCameraIndex].fpsOptions.size() > 2) {
                info << "... (+" << (availableCameras[selectedCameraIndex].fpsOptions.size() - 2) << ")";
                break;
            }
        }

        info << "\nBrightness: " << availableCameras[selectedCameraIndex].minBrightness
            << " - "  << availableCameras[selectedCameraIndex].maxBrightness
            << " | Contrast: " << availableCameras[selectedCameraIndex].minContrast
            << " - " << availableCameras[selectedCameraIndex].maxContrast;
    }

    sf::Text infoText(info.str(), font, 14);
    infoText.setPosition(20.f, 15.f);
    window.draw(infoText);

    hideWindowButton.draw(window);

    character.draw(window);

    window.display();
}

bool Lab4::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab4::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}

void Lab4::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();
    updateBackgroundSize();
    character.setWindowSize(currentWindowSize);
}

void Lab4::updateButtonPositions() {
    float buttonWidth = currentWindowSize.x * 0.18f;
    float buttonHeight = currentWindowSize.y * 0.06f;

    hideWindowButton.setSize(buttonWidth, buttonHeight);

    float buttonX = currentWindowSize.x * 0.75f;
    float buttonY = currentWindowSize.y * 0.15f;

    hideWindowButton.setPosition(buttonX, buttonY);

    unsigned int buttonTextSize = static_cast<unsigned int>(currentWindowSize.y * 0.02f);
    hideWindowButton.setTextSize(buttonTextSize);
}

void Lab4::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}