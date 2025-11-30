#include "Lab2.hpp"


void Lab2::loadPCIData() {
    pciDevices.clear();
    pciInfoLines.clear();

    std::string fullPath = "C:\\3course\\Int\\vmware\\winda\\pciTxt\\pci.txt";
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        // Try alternative paths
        std::vector<std::string> alternativePaths = {
            "pci.txt",
            "resources/pci.txt",
            "../resources/pci.txt",
            "../../resources/pci.txt"
        };

        for (const auto& path : alternativePaths) {
            file.open(path);
            if (file.is_open()) break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "Failed to open pci.txt file" << std::endl;
        pciInfoLines.push_back("PCI data file not found");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        pciInfoLines.push_back(line);
    }
    file.close();

    parsePCIDevices();
}

void Lab2::parsePCIDevices() {
    for (const auto& line : pciInfoLines) {
        if (line.find("Bus") != std::string::npos && line.find("VendorID") != std::string::npos) {
            PCIDevice device;

            // Parse bus
            size_t busPos = line.find("Bus");
            if (busPos != std::string::npos) {
                size_t commaPos = line.find(",", busPos);
                if (commaPos != std::string::npos) {
                    device.bus = line.substr(busPos + 3, commaPos - busPos - 3);
                    // Remove spaces
                    device.bus.erase(std::remove(device.bus.begin(), device.bus.end(), ' '), device.bus.end());
                }
            }

            // Parse device
            size_t devicePos = line.find("Device");
            if (devicePos != std::string::npos) {
                size_t commaPos = line.find(",", devicePos);
                if (commaPos != std::string::npos) {
                    device.device = line.substr(devicePos + 6, commaPos - devicePos - 6);
                    device.device.erase(std::remove(device.device.begin(), device.device.end(), ' '), device.device.end());
                }
            }

            // Parse function
            size_t funcPos = line.find("Func");
            if (funcPos != std::string::npos) {
                size_t arrowPos = line.find("->", funcPos);
                if (arrowPos != std::string::npos) {
                    device.function = line.substr(funcPos + 4, arrowPos - funcPos - 4);
                    device.function.erase(std::remove(device.function.begin(), device.function.end(), ' '), device.function.end());
                }
            }

            // Parse VendorID
            size_t vendorPos = line.find("VendorID:");
            if (vendorPos != std::string::npos) {
                size_t commaPos = line.find(",", vendorPos);
                if (commaPos != std::string::npos) {
                    device.vendorID = line.substr(vendorPos + 9, commaPos - vendorPos - 9);
                    device.vendorID.erase(std::remove(device.vendorID.begin(), device.vendorID.end(), ' '), device.vendorID.end());
                }
            }

            // Parse DeviceID
            size_t deviceIDPos = line.find("DeviceID:");
            if (deviceIDPos != std::string::npos) {
                device.deviceID = line.substr(deviceIDPos + 9);
                device.deviceID.erase(std::remove(device.deviceID.begin(), device.deviceID.end(), ' '), device.deviceID.end());
            }

            if (!device.bus.empty() && !device.device.empty() && !device.function.empty()) {
                pciDevices.push_back(device);
            }
        }
    }
}

bool Lab2::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab2::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}

void Lab2::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();
    updateBackgroundSize();
    character.setWindowSize(currentWindowSize);
}

void Lab2::updateButtonPositions() {
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

void Lab2::initializeButtons() {
    updateButtonPositions();

    prevButton.setOnClick([this]() {
        if (currentPage > 0) {
            currentPage--;
            updateDeviceDisplay();
        }
        });

    nextButton.setOnClick([this]() {
        int totalPages = (pciDevices.size() + devicesPerPage - 1) / devicesPerPage;
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
    devicesPerPage = 15; 
    updateDeviceDisplay();
}

void Lab2::updateDeviceDisplay() {
    int totalPages = (pciDevices.size() + devicesPerPage - 1) / devicesPerPage;

    prevButton.setEnabled(currentPage > 0);
    nextButton.setEnabled(currentPage < totalPages - 1);

    prevButton.setText("Previous");
    nextButton.setText("Next");
}

void Lab2::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}

void Lab2::handleEvents(bool& running) {
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
                int totalPages = (pciDevices.size() + devicesPerPage - 1) / devicesPerPage;
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
                int totalPages = (pciDevices.size() + devicesPerPage - 1) / devicesPerPage;
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

void Lab2::render() {
    window.clear(sf::Color(40, 40, 40));
    if (backgroundTexture.getSize().x > 0) {
        window.draw(backgroundSprite);
    }

    character.draw(window);

    // Background for table
    sf::RectangleShape tableBackground(sf::Vector2f(currentWindowSize.x * 0.9f, currentWindowSize.y * 0.6f));
    tableBackground.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.1f);
    tableBackground.setFillColor(sf::Color(0, 0, 0, 200));
    tableBackground.setOutlineColor(sf::Color::White);
    tableBackground.setOutlineThickness(1.f);
    window.draw(tableBackground);

    unsigned int titleSize = static_cast<unsigned int>(currentWindowSize.y * 0.04f);
    unsigned int headerSize = static_cast<unsigned int>(currentWindowSize.y * 0.030f);
    unsigned int dataSize = static_cast<unsigned int>(currentWindowSize.y * 0.025f);

    // Title
    sf::Text title;
    title.setFont(font);
    title.setString("Laboratory Work 2 - PCI Devices Table");
    title.setCharacterSize(titleSize);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(currentWindowSize.x * 0.05f, currentWindowSize.y * 0.02f);
    window.draw(title);

    // Table headers
    float startX = currentWindowSize.x * 0.07f;
    float startY = currentWindowSize.y * 0.12f;
    float columnWidth = currentWindowSize.x * 0.16f;
    float rowHeight = currentWindowSize.y * 0.035f;

    // Headers
    std::vector<std::string> headers = { "Bus", "Device", "Function", "VendorID", "DeviceID" };
    std::vector<float> columnOffsets = { 0, columnWidth, columnWidth * 2, columnWidth * 3, columnWidth * 4 };

    for (size_t i = 0; i < headers.size(); ++i) {
        sf::Text header;
        header.setFont(font);
        header.setString(headers[i]);
        header.setCharacterSize(headerSize);
        header.setFillColor(sf::Color::Yellow);
        header.setStyle(sf::Text::Bold);
        header.setPosition(startX + columnOffsets[i], startY);
        window.draw(header);
    }

    // Draw horizontal line under headers
    sf::RectangleShape headerLine(sf::Vector2f(columnWidth * 5, 1.f));
    headerLine.setPosition(startX, startY + rowHeight);
    headerLine.setFillColor(sf::Color::White);
    window.draw(headerLine);

    // Display PCI devices in table format
    int startIndex = currentPage * devicesPerPage;
    int endIndex = std::min(startIndex + devicesPerPage, static_cast<int>(pciDevices.size()));

    for (int i = startIndex; i < endIndex; ++i) {
        const auto& device = pciDevices[i];
        float yPos = startY + rowHeight + (i - startIndex) * rowHeight;

        // Alternate row colors for better readability
        sf::Color rowColor = (i % 2 == 0) ? sf::Color::White : sf::Color(200, 200, 200);

        // Bus
        sf::Text busText;
        busText.setFont(font);
        busText.setString(device.bus);
        busText.setCharacterSize(dataSize);
        busText.setFillColor(rowColor);
        busText.setPosition(startX + columnOffsets[0], yPos);
        window.draw(busText);

        // Device
        sf::Text deviceText;
        deviceText.setFont(font);
        deviceText.setString(device.device);
        deviceText.setCharacterSize(dataSize);
        deviceText.setFillColor(rowColor);
        deviceText.setPosition(startX + columnOffsets[1], yPos);
        window.draw(deviceText);

        // Function
        sf::Text funcText;
        funcText.setFont(font);
        funcText.setString(device.function);
        funcText.setCharacterSize(dataSize);
        funcText.setFillColor(rowColor);
        funcText.setPosition(startX + columnOffsets[2], yPos);
        window.draw(funcText);

        // VendorID
        sf::Text vendorText;
        vendorText.setFont(font);
        vendorText.setString(device.vendorID);
        vendorText.setCharacterSize(dataSize);
        vendorText.setFillColor(rowColor);
        vendorText.setPosition(startX + columnOffsets[3], yPos);
        window.draw(vendorText);

        // DeviceID
        sf::Text deviceIDText;
        deviceIDText.setFont(font);
        deviceIDText.setString(device.deviceID);
        deviceIDText.setCharacterSize(dataSize);
        deviceIDText.setFillColor(rowColor);
        deviceIDText.setPosition(startX + columnOffsets[4], yPos);
        window.draw(deviceIDText);
    }

    // Show message if no devices
    if (pciDevices.empty()) {
        sf::Text noDevicesText;
        noDevicesText.setFont(font);
        noDevicesText.setString("No PCI devices found or failed to load data");
        noDevicesText.setCharacterSize(headerSize);
        noDevicesText.setFillColor(sf::Color::Red);
        noDevicesText.setPosition(startX, startY + rowHeight);
        window.draw(noDevicesText);
    }

    // Page and device count info
    int totalPages = (pciDevices.size() + devicesPerPage - 1) / devicesPerPage;

    sf::Text pageInfo;
    pageInfo.setFont(font);
    pageInfo.setString("Page " + std::to_string(currentPage + 1) + " of " + std::to_string(totalPages) +
        " | Devices: " + std::to_string(pciDevices.size()) +
        " | Showing: " + std::to_string(startIndex + 1) + "-" + std::to_string(endIndex));
    pageInfo.setCharacterSize(headerSize);
    pageInfo.setFillColor(sf::Color::Black);
    title.setStyle(sf::Text::Bold);
    pageInfo.setPosition(startX, currentWindowSize.y * 0.06f);
    window.draw(pageInfo);

    // Navigation hints


    // Buttons
    prevButton.draw(window);
    nextButton.draw(window);

    window.display();
}

void Lab2::run() {
    sf::Clock clock;
    bool running = true;

    loadPCIData();
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