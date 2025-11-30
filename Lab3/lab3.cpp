#include "Lab3.hpp"


void Lab3::loadDiskData() {
    diskDevices.clear();
    diskInfoLines.clear();

    std::string fullPath = "C:\\3course\\Int\\vmware\\winda\\pciTxt\\discks.txt";
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        // Try alternative paths
        std::vector<std::string> alternativePaths = {
            "discks.txt",
            "resources/discks.txt",
            "../resources/discks.txt",
            "../../resources/discks.txt"
        };

        for (const auto& path : alternativePaths) {
            file.open(path);
            if (file.is_open()) break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open discks.txt file" << std::endl;
        diskInfoLines.push_back("Disk data file not found");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        diskInfoLines.push_back(line);
    }
    file.close();

    parseDiskDevices();
}

void Lab3::parseDiskDevices() {
    DiskDevice currentDevice;
    bool newDevice = false;

    for (const auto& line : diskInfoLines) {
        if (line.find("Model:") != std::string::npos) {
            // Save previous device if exists
            if (newDevice) {
                diskDevices.push_back(currentDevice);
                currentDevice = DiskDevice();
            }
            newDevice = true;
            currentDevice.model = line.substr(line.find(":") + 2);
        }
        else if (line.find("Serial:") != std::string::npos) {
            currentDevice.serial = line.substr(line.find(":") + 2);
        }
        else if (line.find("Firmware:") != std::string::npos) {
            currentDevice.firmware = line.substr(line.find(":") + 2);
        }
        else if (line.find("Type:") != std::string::npos) {
            currentDevice.type = line.substr(line.find(":") + 2);
        }
        else if (line.find("Interface:") != std::string::npos) {
            currentDevice.inter = line.substr(line.find(":") + 2);
        }
        else if (line.find("Total Physical Size:") != std::string::npos) {
            currentDevice.totalSize = line.substr(line.find(":") + 2);
        }
        else if (line.find("Used Space:") != std::string::npos) {
            currentDevice.usedSpace = line.substr(line.find(":") + 2);
        }
        else if (line.find("Free Space:") != std::string::npos) {
            currentDevice.freeSpace = line.substr(line.find(":") + 2);
        }
        else if (line.find("Supported Modes:") != std::string::npos) {
            currentDevice.supportedModes = line.substr(line.find(":") + 2);
            // This is the last property, push the device
            diskDevices.push_back(currentDevice);
            currentDevice = DiskDevice();
            newDevice = false;
        }
    }

    // Push the last device if file ends unexpectedly
    if (newDevice) {
        diskDevices.push_back(currentDevice);
    }
}

bool Lab3::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab3::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}

void Lab3::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();
    updateBackgroundSize();
    character.setWindowSize(currentWindowSize);
}

void Lab3::updateButtonPositions() {
    float buttonWidth = currentWindowSize.x * 0.15f;
    float buttonHeight = currentWindowSize.y * 0.06f;

    // Navigation buttons
    prevButton.setSize(buttonWidth, buttonHeight);
    nextButton.setSize(buttonWidth, buttonHeight);

    prevButton.setPosition(
        currentWindowSize.x * 0.3f - buttonWidth / 2,
        currentWindowSize.y * 0.85f - buttonHeight / 2
    );

    nextButton.setPosition(
        currentWindowSize.x * 0.7f - buttonWidth / 2,
        currentWindowSize.y * 0.85f - buttonHeight / 2
    );

    unsigned int buttonTextSize = static_cast<unsigned int>(currentWindowSize.y * 0.022f);
    prevButton.setTextSize(buttonTextSize);
    nextButton.setTextSize(buttonTextSize);
}

void Lab3::initializeButtons() {
    updateButtonPositions();

    prevButton.setOnClick([this]() {
        if (currentPage > 0) {
            currentPage--;
            updateDeviceDisplay();
        }
        });

    nextButton.setOnClick([this]() {
        int totalPages = diskDevices.size(); // One device per page
        if (currentPage < totalPages - 1) {
            currentPage++;
            updateDeviceDisplay();
        }
        });

    if (!character.loadTextures("character_lab1_normal.png", "character_lab1_normal.png", "walk.png")) {
        std::cout << "Using default character texture" << std::endl;
    }

    character.setInitialScale(0.7f);
    character.setBaseAnimationSpeed(0.1f);
    character.setPosition(currentWindowSize.x * 0.1f, currentWindowSize.y * 0.7f);
    character.setRandomMovement();

    currentPage = 0;
    devicesPerPage = 1; // One device per page
    updateDeviceDisplay();
}

void Lab3::updateDeviceDisplay() {
    int totalPages = diskDevices.size(); // One device per page

    prevButton.setEnabled(currentPage > 0);
    nextButton.setEnabled(currentPage < totalPages - 1);

    prevButton.setText("Previous");
    nextButton.setText("Next");
}

void Lab3::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}

void Lab3::handleEvents(bool& running) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
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
        else if (event.type == sf::Event::KeyPressed) {
            // Keyboard navigation
            if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                if (currentPage > 0) {
                    currentPage--;
                    updateDeviceDisplay();
                }
            }
            else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                int totalPages = diskDevices.size();
                if (currentPage < totalPages - 1) {
                    currentPage++;
                    updateDeviceDisplay();
                }
            }
            else if (event.key.code == sf::Keyboard::Home) {
                currentPage = 0;
                updateDeviceDisplay();
            }
            else if (event.key.code == sf::Keyboard::End) {
                int totalPages = diskDevices.size();
                currentPage = totalPages - 1;
                updateDeviceDisplay();
            }
        }
        prevButton.handleEvent(event, window);
        nextButton.handleEvent(event, window);

        if (event.type == sf::Event::MouseMoved) {
            currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            mouseOverPrevButton = prevButton.isMouseOver(currentMousePos);
            mouseOverNextButton = nextButton.isMouseOver(currentMousePos);

            if (mouseOverPrevButton && prevButton.isEnabledF()) {
                character.setButtonTargetLeft(prevButton.getPosition(), prevButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverNextButton && nextButton.isEnabledF()) {
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

void Lab3::render() {
    window.clear(sf::Color(40, 40, 40));
    if (backgroundTexture.getSize().x > 0) {
        window.draw(backgroundSprite);
    }

    character.draw(window);

    // Background for table
    sf::RectangleShape tableBackground(sf::Vector2f(currentWindowSize.x * 0.9f, currentWindowSize.y * 0.45f));
    tableBackground.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.1f);
    tableBackground.setFillColor(sf::Color(0, 0, 0, 200));
    tableBackground.setOutlineColor(sf::Color::White);
    tableBackground.setOutlineThickness(1.f);
    window.draw(tableBackground);

    unsigned int titleSize = static_cast<unsigned int>(currentWindowSize.y * 0.04f);
    unsigned int headerSize = static_cast<unsigned int>(currentWindowSize.y * 0.025f);
    unsigned int dataSize = static_cast<unsigned int>(currentWindowSize.y * 0.030f);

    // Title
    sf::Text title;
    title.setFont(font);
    title.setString("Laboratory Work 3 - Disk Devices Information");
    title.setCharacterSize(titleSize);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.02f);
    window.draw(title);

    // Display current disk device information
    if (!diskDevices.empty() && currentPage < diskDevices.size()) {
        const auto& device = diskDevices[currentPage];

        float startX = currentWindowSize.x * 0.07f;
        float startY = currentWindowSize.y * 0.12f;
        float columnWidth = currentWindowSize.x * 0.16f;
        float rowHeight = currentWindowSize.y * 0.04f;

        // Device header
        sf::Text deviceHeader;
        deviceHeader.setFont(font);
        deviceHeader.setString("Disk Device " + std::to_string(currentPage + 1) + " of " + std::to_string(diskDevices.size()));
        deviceHeader.setCharacterSize(headerSize);
        deviceHeader.setFillColor(sf::Color::Cyan);
        deviceHeader.setStyle(sf::Text::Bold);
        deviceHeader.setPosition(startX, startY);
        window.draw(deviceHeader);

        // Device properties
        std::vector<std::pair<std::string, std::string>> properties = {
            {"Model:", device.model},
            {"Serial:", device.serial},
            {"Firmware:", device.firmware},
            {"Type:", device.type},
            {"Interface:", device.inter},
            {"Total Size:", device.totalSize},
            {"Used Space:", device.usedSpace},
            {"Free Space:", device.freeSpace},
            {"Supported Modes:", device.supportedModes}
        };

        for (size_t propIndex = 0; propIndex < properties.size(); ++propIndex) {
            float yPos = startY + rowHeight * (propIndex + 1);

            // Property name
            sf::Text propName;
            propName.setFont(font);
            propName.setString(properties[propIndex].first);
            propName.setCharacterSize(dataSize);
            propName.setFillColor(sf::Color(200, 200, 255));
            propName.setPosition(startX, yPos);
            window.draw(propName);

            // Property value
            sf::Text propValue;
            propValue.setFont(font);
            propValue.setString(properties[propIndex].second);
            propValue.setCharacterSize(dataSize);
            propValue.setFillColor(sf::Color::White);
            propValue.setPosition(startX + columnWidth * 2, yPos);
            window.draw(propValue);
        }
    }

    // Show message if no devices
    if (diskDevices.empty()) {
        float startX = currentWindowSize.x * 0.07f;
        float startY = currentWindowSize.y * 0.12f;

        sf::Text noDevicesText;
        noDevicesText.setFont(font);
        noDevicesText.setString("No disk devices found or failed to load data");
        noDevicesText.setCharacterSize(headerSize);
        noDevicesText.setFillColor(sf::Color::Red);
        noDevicesText.setPosition(startX, startY);
        window.draw(noDevicesText);
    }

    // Page info
    int totalPages = diskDevices.size();

    sf::Text pageInfo;
    pageInfo.setFont(font);
    pageInfo.setString("Page " + std::to_string(currentPage + 1) + " of " + std::to_string(totalPages) +
        " | Total Devices: " + std::to_string(diskDevices.size()));
    pageInfo.setCharacterSize(headerSize);
    pageInfo.setFillColor(sf::Color::Black);
    pageInfo.setStyle(sf::Text::Bold);
    pageInfo.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.06f);
    window.draw(pageInfo);

    // Buttons
    prevButton.draw(window);
    nextButton.draw(window);

    window.display();
}

void Lab3::run() {
    sf::Clock clock;
    bool running = true;

    loadDiskData();
    loadBackground();
    updateLayout();
    initializeButtons();

    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents(running);
        updateCharacter(deltaTime);

        prevButton.update(deltaTime);
        nextButton.update(deltaTime);

        render();
    }
}