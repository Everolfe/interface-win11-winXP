#include "Lab5.hpp"

Lab5::Lab5(sf::RenderWindow& win, sf::Font& f)
    : window(win), font(f),
    character(sf::Vector2f(win.getSize())),
    refreshButton("Refresh (R)", font),
    ejectButton("Eject Selected", font),
    selectedDeviceIndex(-1)
{
    refreshDeviceList();
    previousDevices = devices;
}

void Lab5::pushNotification(const std::wstring& text, NotifType t, int life) {
    std::lock_guard<std::mutex> lk(notifs_mtx);
    notifications.emplace_back(text, t, life);
}

bool Lab5::shouldShowDevice(const std::wstring& desc, bool ejectable) {
    if (ejectable) {
        return true;
    }

    std::wstring lower = desc;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    if (lower.find(L"mouse") != std::wstring::npos)
        return true;
    if (lower.find(L"hid") != std::wstring::npos)  
        return true;

    return false;
}

void Lab5::fillDeviceProperties(DeviceInfo& out, SP_DEVICE_INTERFACE_DATA& ifaceData, HDEVINFO deviceInfoSet) {
    DWORD requiredSize = 0;
    SP_DEVINFO_DATA devInfo = { sizeof(devInfo) };

    //получение размера буфера свойств устройства
    SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &ifaceData, nullptr, 0, &requiredSize, nullptr);
    if (requiredSize == 0) return;

    PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)malloc(requiredSize);
    if (!detail) return;
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    //получение детальной информации
    if (SetupDiGetDeviceInterfaceDetailW(deviceInfoSet, &ifaceData, detail, requiredSize, nullptr, &devInfo)) {
        out.devicePath = detail->DevicePath;

        //получение описни€ устройства
        WCHAR nameBuf[512] = { 0 };
        if (SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &devInfo, SPDRP_DEVICEDESC, nullptr,
            (PBYTE)nameBuf, sizeof(nameBuf), nullptr)) {
            out.name = nameBuf;
        }
        else {
            out.name = L"USB device";
        }

        out.devInst = devInfo.DevInst;

        //получение свойств
        DWORD caps = 0;
        if (SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &devInfo, SPDRP_CAPABILITIES, nullptr,
            (PBYTE)&caps, sizeof(caps), nullptr)) {
            out.ejectable = (caps & CM_DEVCAP_REMOVABLE) != 0; // ¬озможность извлечени€
        }

        if (out.name.empty() || out.name == L"USB device") {
            WCHAR friendlyNameBuf[512] = { 0 };
            if (SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &devInfo, SPDRP_FRIENDLYNAME, nullptr,
                (PBYTE)friendlyNameBuf, sizeof(friendlyNameBuf), nullptr)) {
                out.name = friendlyNameBuf;
            }
        }
    }
    free(detail);
}

void Lab5::detectDeviceChanges(const std::vector<DeviceInfo>& oldDevices, const std::vector<DeviceInfo>& newDevices) {
    for (const auto& newDev : newDevices) {
        bool found = false;
        for (const auto& oldDev : oldDevices) {
            if (newDev.getDeviceKey() == oldDev.getDeviceKey()) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::wstringstream ss;
            ss << L"Connected: " << newDev.name;
            pushNotification(ss.str(), NotifType::Info);
        }
    }

    for (const auto& oldDev : oldDevices) {
        bool found = false;
        for (const auto& newDev : newDevices) {
            if (oldDev.getDeviceKey() == newDev.getDeviceKey()) {
                found = true;
                break;
            }
        }
        if (!found) {
            if (oldDev.requestedEject) {
                std::wstringstream ss;
                ss << L"Safe eject: " << oldDev.name;
                pushNotification(ss.str(), NotifType::Success);
            }
            else {
                std::wstringstream ss;
                ss << L"Unsafe remove: " << oldDev.name;
                pushNotification(ss.str(), NotifType::Warning);
            }
        }
    }
}

void Lab5::refreshDeviceList() {
    // получение usb устройств
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        pushNotification(L"Failed to get device list", NotifType::Error);
        return;
    }

    std::vector<DeviceInfo> newList;
    SP_DEVICE_INTERFACE_DATA ifaceData;
    ifaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    int totalDevices = 0;
    int filteredDevices = 0;

    for (DWORD idx = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_USB_DEVICE, idx, &ifaceData); ++idx) {
        totalDevices++;
        DeviceInfo di;
        fillDeviceProperties(di, ifaceData, deviceInfoSet);
        if (di.devicePath.empty()) continue;

        if (shouldShowDevice(di.name, di.ejectable)) {
            newList.push_back(std::move(di));
            filteredDevices++;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    std::wcout << L"Total USB devices found: " << totalDevices
        << L", filtered: " << filteredDevices << std::endl;

    {
        std::lock_guard<std::mutex> lk(devices_mtx);

        detectDeviceChanges(devices, newList);

        for (auto& newDev : newList) {
            for (const auto& oldDev : devices) {
                if (newDev.getDeviceKey() == oldDev.getDeviceKey()) {
                    newDev.requestedEject = oldDev.requestedEject;
                }
            }
        }

        devices = std::move(newList);

        // Reset selection if invalid
        if (selectedDeviceIndex >= static_cast<int>(devices.size())) {
            selectedDeviceIndex = -1;
        }
    }
}

bool Lab5::tryEjectByDevInst(DEVINST devInst, const std::wstring& name) {
    if (devInst == 0) {
        pushNotification(L"Cannot eject: unknown device", NotifType::Error);
        return false;
    }

    // запрос на извлечение
    CONFIGRET cr = CM_Request_Device_EjectW(devInst, nullptr, nullptr, 0, 0);
    if (cr == CR_SUCCESS) {
        std::wstringstream ss;
        ss << L"Eject requested: " << name;
        pushNotification(ss.str(), NotifType::Info);
        return true;
    }
    else {
        std::wstringstream ss;
        ss << L"Eject denied: " << name << L" (code 0x" << std::hex << cr << L")";
        pushNotification(ss.str(), NotifType::Error);
        return false;
    }
}

sf::Color Lab5::notifColor(NotifType t) {
    switch (t) {
    case NotifType::Success: return sf::Color(80, 200, 120);
    case NotifType::Warning: return sf::Color(240, 180, 40);
    case NotifType::Error:   return sf::Color(220, 70, 70);
    default:                 return sf::Color(200, 200, 200);
    }
}

void Lab5::drawNotifications(sf::RenderWindow& window) {
    std::lock_guard<std::mutex> lk(notifs_mtx);

    // ”дал€ем просроченные уведомлени€
    notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
        [](const Notification& n) { return n.expired(); }), notifications.end());

    // ѕозици€ уведомлений справа от списка устройств
    float deviceListWidth = currentWindowSize.x - 400; // Ўирина списка устройств
    float notificationX = deviceListWidth + 25.f; // ќтступ от списка устройств
    float notificationY = 20.f; // ¬ерхний край
    float notificationWidth = currentWindowSize.x - deviceListWidth - 60.f; // Ўирина с отступами

    // ѕоказываем только последние 3 уведомлени€ (самые новые)
    int startIndex = std::max(0, static_cast<int>(notifications.size()) - 3);
    int visibleCount = static_cast<int>(notifications.size()) - startIndex;

    for (int i = 0; i < visibleCount; ++i) {
        int notificationIndex = startIndex + i;
        auto& n = notifications[notificationIndex];

        float frac = n.age_fraction();
        float alpha = 1.f;
        if (frac > 0.7f) alpha = 1.f - (frac - 0.7f) / 0.3f;
        int a = static_cast<int>(alpha * 230.0f);
        a = (a < 0) ? 0 : (a > 230 ? 230 : a);

        sf::RectangleShape rect(sf::Vector2f(notificationWidth, 40.f));
        rect.setPosition(notificationX, notificationY + i * 48.f);
        sf::Color bg = notifColor(n.type);
        bg.a = static_cast<sf::Uint8>(a);
        rect.setFillColor(bg);
        rect.setOutlineColor(sf::Color(40, 40, 40));
        rect.setOutlineThickness(1.f);
        window.draw(rect);

        // »спользуем правильное преобразование в UTF-8
        std::string text = wstringToUTF8(n.text);
        sf::Text txt(sf::String::fromUtf8(text.begin(), text.end()), font, 16);

        txt.setPosition(notificationX + 10.f, notificationY + 8.f + i * 48.f);
        txt.setFillColor(sf::Color::Black);
        window.draw(txt);
    }
}

void Lab5::drawDeviceList(sf::RenderWindow& window) {
    std::lock_guard<std::mutex> lk(devices_mtx);

    sf::Text title("USB Devices - Click to select, then press Eject", font, 18);
    title.setPosition(20, 20);
    title.setFillColor(sf::Color(230, 230, 230));
    window.draw(title);

    // ќтображаем количество найденных устройств
    std::string countText = "Found: " + std::to_string(devices.size()) + " devices";
    sf::Text count(countText, font, 14);
    count.setPosition(20, 45);
    count.setFillColor(sf::Color(180, 180, 180));
    window.draw(count);

    if (devices.empty()) {
        sf::Text none("(No USB devices detected)", font, 18);
        none.setPosition(20, 80);
        none.setFillColor(sf::Color(180, 180, 180));
        window.draw(none);
        return;
    }

    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& d = devices[i];

        // ѕреобразуем им€ устройства в UTF-8
        std::string name = wstringToUTF8(d.name);
        std::stringstream ss;
        ss << (i + 1) << ") " << name;

        if (d.requestedEject) ss << " - Eject Requested";
        std::string utf8 = ss.str();
        sf::Text t(sf::String::fromUtf8(utf8.begin(), utf8.end()), font, 16);

        t.setPosition(20, 80 + static_cast<int>(i) * 30);

        // Highlight selected device
        if (static_cast<int>(i) == selectedDeviceIndex) {
            t.setFillColor(sf::Color::Yellow);
            t.setStyle(sf::Text::Bold);

            // Draw selection background
            sf::RectangleShape highlight(sf::Vector2f(currentWindowSize.x - 400 - 40, 25));
            highlight.setPosition(15, 77 + static_cast<int>(i) * 30);
            highlight.setFillColor(sf::Color(100, 100, 100, 100));
            window.draw(highlight);
        }
        else {
            t.setFillColor(sf::Color::White);
        }

        window.draw(t);
    }
}


void Lab5::run() {
    LabWindow::activeWindow = this;

    loadBackground();
    initializeButtons();

    sf::Clock clock;
    bool running = true;

    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents(running);
        updateCharacter(deltaTime);

        refreshButton.update(deltaTime);
        ejectButton.update(deltaTime);

        render();
    }
    LabWindow::activeWindow = nullptr;
}

void Lab5::initializeButtons() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();

    refreshButton.setOnClick([this]() {
        refreshDeviceList();
        pushNotification(L"Manual refresh", NotifType::Info);
        });

    ejectButton.setOnClick([this]() {
        if (selectedDeviceIndex >= 0 && selectedDeviceIndex < static_cast<int>(devices.size())) {
            DeviceInfo devCopy;
            {
                std::lock_guard<std::mutex> lk(devices_mtx);
                devices[selectedDeviceIndex].requestedEject = true;
                devCopy = devices[selectedDeviceIndex];
            }

            std::thread([this, devCopy]() {
                bool ok = tryEjectByDevInst(devCopy.devInst, devCopy.name);

                if (!ok) {
                    std::lock_guard<std::mutex> lk(devices_mtx);
                    for (auto& d : devices) {
                        if (d.devicePath == devCopy.devicePath) {
                            d.requestedEject = false;
                        }
                    }
                }
                }).detach(); 

            pushNotification(L"Ejecting device...", NotifType::Info);
        }
        else {
            pushNotification(L"No device selected", NotifType::Warning);
        }
        });

    if (!character.loadTextures("character_lab1_normal.png", "character_lab1_normal.png", "walk.png")) {
        std::cout << "Using default character texture" << std::endl;
    }

    character.setInitialScale(0.7f);
    character.setBaseAnimationSpeed(0.1f);
    character.setPosition(currentWindowSize.x * 0.1f, currentWindowSize.y * 0.7f);
    character.setRandomMovement();
}

void Lab5::handleEvents(bool& running) {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
        }
        else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                running = false;
            }
            else if (event.key.code == sf::Keyboard::R) {
                refreshDeviceList();
                pushNotification(L"Manual refresh", NotifType::Info);
            }
            else if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                int idx = event.key.code - sf::Keyboard::Num1;
                std::lock_guard<std::mutex> lk(devices_mtx);
                if (idx >= 0 && idx < static_cast<int>(devices.size())) {
                    selectedDeviceIndex = idx;
                }
            }
        }
        else if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                // Check if clicking on device list
                if (mousePos.y >= 70 && mousePos.y <= 70 + devices.size() * 30) {
                    int clickedIndex = static_cast<int>((mousePos.y - 70) / 30);
                    std::lock_guard<std::mutex> lk(devices_mtx);
                    if (clickedIndex >= 0 && clickedIndex < static_cast<int>(devices.size())) {
                        selectedDeviceIndex = clickedIndex;
                    }
                }
            }
        }
        else if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0, 0, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
            window.setView(sf::View(visibleArea));
            updateLayout();
        }

        refreshButton.handleEvent(event, window);
        ejectButton.handleEvent(event, window);

        if (event.type == sf::Event::MouseMoved) {
            currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            mouseOverRefreshButton = refreshButton.isMouseOver(currentMousePos);
            mouseOverEjectButton = ejectButton.isMouseOver(currentMousePos);

            if (mouseOverRefreshButton) {
                character.setButtonTargetLeft(refreshButton.getPosition(), refreshButton.getSize());
                character.setPointing(true);
            }
            else if (mouseOverEjectButton) {
                character.setButtonTargetLeft(ejectButton.getPosition(), ejectButton.getSize());
                character.setPointing(true);
            }
            else {
                character.setPointing(false);
                character.moveToMouse(currentMousePos);
            }
        }
    }
}

void Lab5::render() {
    window.clear(sf::Color(30, 32, 40));

    if (backgroundTexture.getSize().x > 0) {
        window.draw(backgroundSprite);
    }

    // Device list background (лева€ часть)
    float deviceListWidth = currentWindowSize.x - 400;
    sf::RectangleShape listBg(sf::Vector2f(deviceListWidth, currentWindowSize.y - 450));
    listBg.setPosition(18, 10);
    listBg.setFillColor(sf::Color(0, 0, 0, 150));
    listBg.setOutlineColor(sf::Color::White);
    listBg.setOutlineThickness(1.f);
    window.draw(listBg);

    // Notifications background (права€ часть)
    float notificationX = deviceListWidth + 20.f;
    float notificationWidth = currentWindowSize.x - deviceListWidth - 40.f;
    sf::RectangleShape notifBg(sf::Vector2f(notificationWidth, currentWindowSize.y - 450));
    notifBg.setPosition(notificationX, 10);
    notifBg.setFillColor(sf::Color(0, 0, 0, 120));
    notifBg.setOutlineColor(sf::Color::White);
    notifBg.setOutlineThickness(1.f);
    window.draw(notifBg);

    character.draw(window);

    drawDeviceList(window);
    drawNotifications(window);

    refreshButton.draw(window);
    ejectButton.draw(window);


    window.display();
}

bool Lab5::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab5::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}

void Lab5::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());
    updateButtonPositions();
    updateBackgroundSize();
    character.setWindowSize(currentWindowSize);
}

void Lab5::updateButtonPositions() {
    float buttonWidth = currentWindowSize.x * 0.15f;
    float buttonHeight = currentWindowSize.y * 0.06f;

    refreshButton.setSize(buttonWidth, buttonHeight);
    ejectButton.setSize(buttonWidth, buttonHeight);

    refreshButton.setPosition(
        currentWindowSize.x * 0.3f - buttonWidth / 2,
        currentWindowSize.y * 0.85f - buttonHeight / 2
    );

    ejectButton.setPosition(
        currentWindowSize.x * 0.7f - buttonWidth / 2,
        currentWindowSize.y * 0.85f - buttonHeight / 2
    );

    unsigned int buttonTextSize = static_cast<unsigned int>(currentWindowSize.y * 0.022f);
    refreshButton.setTextSize(buttonTextSize);
    ejectButton.setTextSize(buttonTextSize);
}

void Lab5::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}