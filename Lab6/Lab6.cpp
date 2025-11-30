#include "Lab6.hpp"

std::string OpenFileDialog() {
    //cтруктура для диалога открытия файла (ANSI версия)
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.TXT\0Image Files\0*.BMP;*.JPG;*.PNG\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST; // только существующие пути и файлы

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

Lab6::Lab6(sf::RenderWindow& win, sf::Font& f)
    : window(win), font(f),
    character(sf::Vector2f(win.getSize())),
    refreshButton("Refresh Devices", font),
    serverButton("Run as Server", font),
    chooseFileButton("Choose File", font),
    sendButton("Send File", font),
    nextButton("Next >", font),        // Инициализация новых кнопок
    previousButton("< Previous", font) // Инициализация новых кнопок
{
    refreshDeviceList();
    updateStatus("Ready - Refresh devices to see available Bluetooth devices");
}

void Lab6::refreshDeviceList() {
    updateStatus("Scanning for Bluetooth devices...");

    std::thread([this]() {
        std::vector<BluetoothDevice> newDevices;
        ListBluetoothDevices(newDevices);

        std::lock_guard<std::mutex> lk(devices_mtx);
        bluetoothDevices = std::move(newDevices);

        if (bluetoothDevices.empty()) {
            updateStatus("No Bluetooth devices found. Make sure Bluetooth is enabled.");
        }
        else {
            updateStatus("Found " + std::to_string(bluetoothDevices.size()) + " Bluetooth devices");
        }
        }).detach(); 
}

void Lab6::updateStatus(const std::string& message) {
    statusMessage = message;
    std::cout << "Status: " << message << std::endl;
}

std::string Lab6::getFileSizeString(const std::string& filePath) {
    try {
        auto fileSize = std::filesystem::file_size(filePath);
        if (fileSize < 1024) {
            return std::to_string(fileSize) + " bytes";
        }
        else if (fileSize < 1024 * 1024) {
            return std::to_string(fileSize / 1024) + " KB";
        }
        else {
            return std::to_string(fileSize / (1024 * 1024)) + " MB";
        }
    }
    catch (...) {
        return "Unknown size";
    }
}

void Lab6::sendFileToDevice() {
    if (selectedDeviceIndex < 0 || selectedDeviceIndex >= static_cast<int>(bluetoothDevices.size())) {
        updateStatus("Error: No device selected");
        return;
    }

    if (selectedFilePath.empty()) {
        updateStatus("Error: No file selected");
        return;
    }

    if (!std::filesystem::exists(selectedFilePath)) {
        updateStatus("Error: Selected file does not exist");
        return;
    }

    BluetoothDevice& device = bluetoothDevices[selectedDeviceIndex];

    if (device.targetType == UNKNOWN) {
        updateStatus("Warning: Unknown device type - file transfer may not work");
    }

    updateStatus("Sending file to " + device.addressStr + "...");

    std::thread([this, device]() {
        SendFile(selectedFilePath, device);
        updateStatus("File sent successfully to " + device.addressStr);
        }).detach();
}

void Lab6::runServer() {
    std::thread([this]() {
        updateStatus("Server running - waiting for connections...");
        RunServer();
        updateStatus("File received successfully");
        }).detach();
}

void Lab6::stopServer() {
    // Здесь можно добавить логику остановки сервера
    updateStatus("Server stopped");
}

bool Lab6::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab6::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}

void Lab6::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();
    updateBackgroundSize();
    character.setWindowSize(currentWindowSize);
}

void Lab6::updateButtonPositions() {
    float buttonWidth = currentWindowSize.x * 0.18f;
    float buttonHeight = currentWindowSize.y * 0.06f;

    refreshButton.setSize(buttonWidth, buttonHeight);
    serverButton.setSize(buttonWidth, buttonHeight);
    chooseFileButton.setSize(buttonWidth, buttonHeight);
    sendButton.setSize(buttonWidth, buttonHeight);

    // Кнопки пагинации - меньшего размера
    previousButton.setSize(buttonWidth * 0.6f, buttonHeight * 0.8f);
    nextButton.setSize(buttonWidth * 0.6f, buttonHeight * 0.8f);

    float buttonSpacing = currentWindowSize.x * 0.02f;
    float startX = currentWindowSize.x * 0.2f;
    float startY = currentWindowSize.y * 0.75f;

    refreshButton.setPosition(startX, startY);
    serverButton.setPosition(startX + buttonWidth + buttonSpacing, startY);
    chooseFileButton.setPosition(startX + 2 * (buttonWidth + buttonSpacing), startY);
    sendButton.setPosition(startX + 3 * (buttonWidth + buttonSpacing), startY);

    previousButton.setPosition(startX + buttonWidth + buttonSpacing,  startY+50);
    nextButton.setPosition(startX + 2 * (buttonWidth + buttonSpacing), startY+50);

    unsigned int buttonTextSize = static_cast<unsigned int>(currentWindowSize.y * 0.02f);
    refreshButton.setTextSize(buttonTextSize);
    serverButton.setTextSize(buttonTextSize);
    chooseFileButton.setTextSize(buttonTextSize);
    sendButton.setTextSize(buttonTextSize);

    // Меньший размер текста для кнопок пагинации
    previousButton.setTextSize(buttonTextSize);
    nextButton.setTextSize(buttonTextSize);
}

void Lab6::updatePageButtons() {
    // Активируем/деактивируем кнопки в зависимости от текущей страницы
    previousButton.setEnabled(currentPage > 0);
    nextButton.setEnabled((currentPage + 1) * devicesPerPage < bluetoothDevices.size());
}

void Lab6::initializeButtons() {
    updateButtonPositions();

    refreshButton.setOnClick([this]() {
        refreshDeviceList();
        currentPage = 0; // Сбрасываем на первую страницу при обновлении
        updatePageButtons();
        });

    serverButton.setOnClick([this]() {
        isServerRunning = !isServerRunning;
        if (isServerRunning) {
            serverButton.setText("Stop Server");
            runServer();
        }
        else {
            serverButton.setText("Run as Server");
            stopServer();
        }
        });

    chooseFileButton.setOnClick([this]() {
        std::string filePath = OpenFileDialog();
        if (!filePath.empty()) {
            selectedFilePath = filePath;
            updateStatus("Selected file: (" + getFileSizeString(selectedFilePath) + ")");
        }
        else {
            updateStatus("File selection cancelled");
        }
        });

    sendButton.setOnClick([this]() {
        sendFileToDevice();
        });

    // Обработчики для кнопок пагинации
    previousButton.setOnClick([this]() {
        if (currentPage > 0) {
            currentPage--;
            updatePageButtons();
        }
        });

    nextButton.setOnClick([this]() {
        if ((currentPage + 1) * devicesPerPage < bluetoothDevices.size()) {
            currentPage++;
            updatePageButtons();
        }
        });

    if (!character.loadTextures("character_lab1_normal.png", "character_lab1_normal.png", "walk.png")) {
        std::cout << "Using default character texture" << std::endl;
    }

    character.setInitialScale(0.7f);
    character.setBaseAnimationSpeed(0.1f);
    character.setPosition(currentWindowSize.x * 0.1f, currentWindowSize.y * 0.5f);
    character.setRandomMovement();

    updatePageButtons(); // Инициализируем состояние кнопок пагинации
}


void Lab6::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}

void Lab6::handleEvents(bool& running) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
        }
        else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            running = false;
        }
        else if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
            window.setView(sf::View(visibleArea));
            updateLayout();
        }
        else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                std::lock_guard<std::mutex> lk(devices_mtx);

                // Область для клика по устройствам (только видимая страница)
                float listStartX = currentWindowSize.x * 0.06f;
                float listEndX = currentWindowSize.x * 0.9f;
                float listStartY = currentWindowSize.y * 0.15f;

                // Вычисляем видимые устройства
                size_t startIndex = currentPage * devicesPerPage;
                size_t endIndex = std::min(startIndex + devicesPerPage, bluetoothDevices.size());
                size_t visibleCount = endIndex - startIndex;

                float listEndY = listStartY + visibleCount * 30;

                if (mousePos.x >= listStartX && mousePos.x <= listEndX &&
                    mousePos.y >= listStartY && mousePos.y <= listEndY) {

                    // Вычисляем индекс с учетом пагинации
                    int clickedIndex = static_cast<int>((mousePos.y - listStartY) / 30);
                    if (clickedIndex >= 0 && clickedIndex < static_cast<int>(visibleCount)) {
                        selectedDeviceIndex = startIndex + clickedIndex;
                        updateStatus("Selected device: " + bluetoothDevices[selectedDeviceIndex].addressStr +
                            " (" + std::string(bluetoothDevices[selectedDeviceIndex].name.begin(),
                                bluetoothDevices[selectedDeviceIndex].name.end()) + ")");
                    }
                }
            }
        }

        refreshButton.handleEvent(event, window);
        serverButton.handleEvent(event, window);
        chooseFileButton.handleEvent(event, window);
        sendButton.handleEvent(event, window);
        previousButton.handleEvent(event, window); // Обработка новых кнопок
        nextButton.handleEvent(event, window);     // Обработка новых кнопок

        if (event.type == sf::Event::MouseMoved) {
            currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            mouseOverRefreshButton = refreshButton.isMouseOver(currentMousePos);
            mouseOverServerButton = serverButton.isMouseOver(currentMousePos);
            mouseOverChooseFileButton = chooseFileButton.isMouseOver(currentMousePos);
            mouseOverSendButton = sendButton.isMouseOver(currentMousePos);
            mouseOverPreviousButton = previousButton.isMouseOver(currentMousePos);
            mouseOverNextButton = nextButton.isMouseOver(currentMousePos);

            if (mouseOverRefreshButton) {
                character.setButtonTargetLeft(refreshButton.getPosition(), refreshButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverServerButton) {
                character.setButtonTargetLeft(serverButton.getPosition(), serverButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverChooseFileButton) {
                character.setButtonTargetLeft(chooseFileButton.getPosition(), chooseFileButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverSendButton) {
                character.setButtonTargetLeft(sendButton.getPosition(), sendButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverPreviousButton) {
                character.setButtonTargetLeft(previousButton.getPosition(), previousButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverNextButton) {
                character.setButtonTargetLeft(nextButton.getPosition(), nextButton.getSize());
                character.setPointing(true);
            }
            else {
                character.setPointing(false);
                character.moveToMouse(currentMousePos);
            }
        }
    }
}

void Lab6::render() {
    window.clear(sf::Color(40, 40, 40));
    if (backgroundTexture.getSize().x > 0) {
        window.draw(backgroundSprite);
    }

    character.draw(window);

    sf::RectangleShape mainPanel(sf::Vector2f(currentWindowSize.x * 0.9f, currentWindowSize.y * 0.7f));
    mainPanel.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.01f);
    mainPanel.setFillColor(sf::Color(0, 0, 0, 200));
    mainPanel.setOutlineColor(sf::Color::White);
    mainPanel.setOutlineThickness(1.f);
    window.draw(mainPanel);

    unsigned int titleSize = static_cast<unsigned int>(currentWindowSize.y * 0.05f);
    unsigned int headerSize = static_cast<unsigned int>(currentWindowSize.y * 0.03f);
    unsigned int dataSize = static_cast<unsigned int>(currentWindowSize.y * 0.025f);

    sf::Text title;
    title.setFont(font);
    title.setString("Laboratory Work 6 - Bluetooth File Transfer");
    title.setCharacterSize(titleSize);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.02f);
    window.draw(title);

    sf::Text statusText;
    statusText.setFont(font);
    statusText.setString("Status: " + statusMessage);
    statusText.setCharacterSize(headerSize);
    statusText.setFillColor(sf::Color::Yellow);
    statusText.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.07f);
    window.draw(statusText);

    sf::Text devicesHeader;
    devicesHeader.setFont(font);
    devicesHeader.setString("Available Bluetooth Devices (click to select):");
    devicesHeader.setCharacterSize(headerSize);
    devicesHeader.setFillColor(sf::Color::Cyan);
    devicesHeader.setPosition(currentWindowSize.x * 0.07f, currentWindowSize.y * 0.12f);
    window.draw(devicesHeader);

    std::lock_guard<std::mutex> lk(devices_mtx);
    float startY = currentWindowSize.y * 0.15f;

    if (bluetoothDevices.empty()) {
        sf::Text noDevicesText;
        noDevicesText.setFont(font);
        noDevicesText.setString("No Bluetooth devices found. Click 'Refresh Devices' to scan.");
        noDevicesText.setCharacterSize(dataSize);
        noDevicesText.setFillColor(sf::Color(200, 200, 200));
        noDevicesText.setPosition(currentWindowSize.x * 0.07f, startY);
        window.draw(noDevicesText);
    }
    else {
        // Вычисляем диапазон отображаемых устройств
        size_t startIndex = currentPage * devicesPerPage;
        size_t endIndex = std::min(startIndex + devicesPerPage, bluetoothDevices.size());

        // Отображаем информацию о странице
        std::string pageInfo = "Page " + std::to_string(currentPage + 1) +
            " of " + std::to_string((bluetoothDevices.size() + devicesPerPage - 1) / devicesPerPage) +
            " (Devices " + std::to_string(startIndex + 1) + "-" +
            std::to_string(endIndex) + " of " +
            std::to_string(bluetoothDevices.size()) + ")";

        sf::Text pageText;
        pageText.setFont(font);
        pageText.setString(pageInfo);
        pageText.setCharacterSize(dataSize - 2);
        pageText.setFillColor(sf::Color(150, 150, 150));
        pageText.setPosition(currentWindowSize.x * 0.07f, currentWindowSize.y * 0.68f);
        window.draw(pageText);

        for (size_t i = startIndex; i < endIndex; ++i) {
            const auto& device = bluetoothDevices[i];
            float yPos = startY + (i - startIndex) * 30;

            if (static_cast<int>(i) == selectedDeviceIndex) {
                sf::RectangleShape highlight(sf::Vector2f(currentWindowSize.x * 0.8f, 28));
                highlight.setPosition(currentWindowSize.x * 0.06f, yPos - 2);
                highlight.setFillColor(sf::Color(100, 100, 100, 100));
                window.draw(highlight);
            }

            std::string deviceInfo = std::to_string(i + 1) + ") " +
                std::string(device.name.begin(), device.name.end()) +
                " [" + device.addressStr + "] - " +
                device.deviceClass + " (" +
                TargetTypeToString(device.targetType) + ")";

            sf::Text deviceText;
            deviceText.setFont(font);
            deviceText.setString(deviceInfo);
            deviceText.setCharacterSize(dataSize);

            if (device.targetType == UNKNOWN) {
                deviceText.setFillColor(sf::Color(255, 165, 0));
            }
            else {
                deviceText.setFillColor(sf::Color::White);
            }

            deviceText.setPosition(currentWindowSize.x * 0.07f, yPos);
            window.draw(deviceText);
        }
    }

    if (!selectedFilePath.empty()) {
        std::filesystem::path path(selectedFilePath);
        std::string fileName = path.filename().string();

        sf::Text fileInfo;
        fileInfo.setFont(font);
        fileInfo.setString("Selected file: " + fileName + " (" + getFileSizeString(selectedFilePath) + ")");
        fileInfo.setCharacterSize(headerSize);
        fileInfo.setFillColor(sf::Color::Green);
        fileInfo.setPosition(currentWindowSize.x * 0.07f, currentWindowSize.y * 0.65f);
        window.draw(fileInfo);

        sf::Text fullPathInfo;
        fullPathInfo.setFont(font);
        fullPathInfo.setString("Full path: " + selectedFilePath);
        fullPathInfo.setCharacterSize(dataSize - 2);
        fullPathInfo.setFillColor(sf::Color(150, 150, 150));
        fullPathInfo.setPosition(currentWindowSize.x * 0.07f, currentWindowSize.y * 0.68f);
        window.draw(fullPathInfo);
    }

    refreshButton.draw(window);
    serverButton.draw(window);
    chooseFileButton.draw(window);
    sendButton.draw(window);
    previousButton.draw(window); // Отрисовка новых кнопок
    nextButton.draw(window);     // Отрисовка новых кнопок

    window.display();
}

void Lab6::run() {
    sf::Clock clock;
    bool running = true;

    loadBackground();
    updateLayout();
    initializeButtons();

    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents(running);
        updateCharacter(deltaTime);

        refreshButton.update(deltaTime);
        serverButton.update(deltaTime);
        chooseFileButton.update(deltaTime);
        sendButton.update(deltaTime);
        previousButton.update(deltaTime); // Обновление новых кнопок
        nextButton.update(deltaTime);     // Обновление новых кнопок

        render();
    }
}