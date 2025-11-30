#include "Lab1.hpp"


std::string Lab1::GetBatteryType() {
    HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVCLASS_BATTERY,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (DeviceInfoSet == INVALID_HANDLE_VALUE) {
        return "Battery not detected";
    }

    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = {};
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &GUID_DEVCLASS_BATTERY, 0, &DeviceInterfaceData)) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "No battery interface found";
    }

    DWORD cbRequired = 0;
    SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, NULL, 0, &cbRequired, NULL);

    PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
    if (!pdidd) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "Memory allocation failed";
    }

    pdidd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DeviceInterfaceData, pdidd, cbRequired, &cbRequired, NULL)) {
        LocalFree(pdidd);
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "Failed to get device interface detail";
    }

    HANDLE hBattery = CreateFile(
        pdidd->DevicePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hBattery == INVALID_HANDLE_VALUE) {
        LocalFree(pdidd);
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "Failed to open battery device";
    }

    BATTERY_QUERY_INFORMATION BatteryQueryInformation = {};
    DWORD bytesReturned = 0;
    DWORD wait = 0;

    if (!DeviceIoControl(
        hBattery,
        IOCTL_BATTERY_QUERY_TAG,
        &wait,
        sizeof(wait),
        &BatteryQueryInformation.BatteryTag,
        sizeof(BatteryQueryInformation.BatteryTag),
        &bytesReturned,
        NULL) || BatteryQueryInformation.BatteryTag == 0) {
        CloseHandle(hBattery);
        LocalFree(pdidd);
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "Battery tag query failed";
    }

    BatteryQueryInformation.InformationLevel = BatteryInformation;
    BATTERY_INFORMATION BatteryInfo = {};

    if (!DeviceIoControl(
        hBattery,
        IOCTL_BATTERY_QUERY_INFORMATION,
        &BatteryQueryInformation,
        sizeof(BatteryQueryInformation),
        &BatteryInfo,
        sizeof(BatteryInfo),
        &bytesReturned,
        NULL)) {
        CloseHandle(hBattery);
        LocalFree(pdidd);
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
        return "Battery information query failed";
    }

    std::string chemistry;
    for (int i = 0; i < 4; ++i) {
        chemistry += BatteryInfo.Chemistry[i];
    }

    CloseHandle(hBattery);
    LocalFree(pdidd);
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    std::transform(chemistry.begin(), chemistry.end(), chemistry.begin(), ::toupper);

    if (chemistry == "LION") return "Lithium-Ion";
    if (chemistry == "LIIO") return "Lithium-Ion";
    if (chemistry == "NIMH") return "Nickel-Metal Hydride";
    if (chemistry == "NICD") return "Nickel-Cadmium";
    if (chemistry == "PBAC") return "Lead-Acid";
    if (chemistry == "LIMN") return "Lithium-Manganese";

    return chemistry.empty() ? "Unknown chemistry" : chemistry;
}



bool Lab1::loadBackground() {
    if (loadTextureFromResources(backgroundTexture, "background.jpg")) {
        backgroundSprite.setTexture(backgroundTexture);
        updateBackgroundSize();
        std::cout << "Background loaded successfully!" << std::endl;
        return true;
    }

    std::cout << "Failed to load background.jpg. Using default background color." << std::endl;
    return false;
}

void Lab1::updateBackgroundSize() {
    if (backgroundTexture.getSize().x > 0 && backgroundTexture.getSize().y > 0) {
        sf::Vector2f windowSize = static_cast<sf::Vector2f>(window.getSize());
        sf::Vector2u textureSize = backgroundTexture.getSize();

        float scaleX = windowSize.x / textureSize.x;
        float scaleY = windowSize.y / textureSize.y;

        backgroundSprite.setScale(scaleX, scaleY);
    }
}
void Lab1::updateLayout() {
    currentWindowSize = static_cast<sf::Vector2f>(window.getSize());

    updateButtonPositions();
    updateBackgroundSize(); 

    character.setWindowSize(currentWindowSize);
}
void Lab1::updateButtonPositions() {
    float buttonWidth = currentWindowSize.x * 0.25f;
    float buttonHeight = currentWindowSize.y * 0.08f;

    sleepButton.setSize(buttonWidth, buttonHeight);
    hibernateButton.setSize(buttonWidth, buttonHeight);

    sleepButton.setPosition(
        currentWindowSize.x * 0.9f - buttonWidth / 2,
        currentWindowSize.y * 0.4f - buttonHeight / 2
    );

    hibernateButton.setPosition(
        currentWindowSize.x * 0.9f - buttonWidth / 2,
        currentWindowSize.y * 0.55f - buttonHeight / 2
    );

    unsigned int buttonTextSize = static_cast<unsigned int>(currentWindowSize.y * 0.025f);
    sleepButton.setTextSize(buttonTextSize);
    hibernateButton.setTextSize(buttonTextSize);
}
void Lab1::initializeButtons() {
    updateButtonPositions();

    sleepButton.setOnClick([this]() {
        SYSTEM_POWER_CAPABILITIES caps;
        GetPwrCapabilities(&caps);
        if (!caps.SystemS3) {
            std::cout << "Sleep mode is not supported, turn on hibernate..." << std::endl;
        }
        else {
            std::cout << "Entering sleep mode..." << std::endl;
        }
        SetSuspendState(FALSE, FALSE, FALSE);
        });

    hibernateButton.setOnClick([this]() {
        SYSTEM_POWER_CAPABILITIES caps;
        GetPwrCapabilities(&caps);
        if (!caps.SystemS4) {
            std::cout << "Hibernate mode is not supported..." << std::endl;
        }
        else {
            std::cout << "Entering hibernate mode..." << std::endl;
        }
        SetSuspendState(TRUE, FALSE, FALSE);
        });

    if (!character.loadTextures("character_lab1_normal.png", "character_lab1_normal.png","walk.png")) {
        std::cout << "Using default character texture" << std::endl;
    }

    character.setInitialScale(0.7f); 
    character.setBaseAnimationSpeed(0.1f); 
    character.setPosition(currentWindowSize.x * 0.1f, currentWindowSize.y * 0.7f);
    character.setRandomMovement(); 
}

void Lab1::updateCharacter(float deltaTime) {
    character.update(deltaTime);
}

void Lab1::updatePowerMode(const SYSTEM_POWER_STATUS& status) {
    powerModeStr = (status.SystemStatusFlag == 1 || status.SystemStatusFlag & 0x01)
        ? "Battery Saver ON"
        : "Battery Saver OFF";
}

void Lab1::updatePowerSource(const SYSTEM_POWER_STATUS& status) {
    switch (status.ACLineStatus) {
    case 0:
        powerSourceStr = "On Battery";
        onBattery = true;
        break;
    case 1:
        powerSourceStr = "AC Power";
        onBattery = false;
        break;
    default:
        powerSourceStr = "Unknown";
        break;
    }
}

void Lab1::updateBatteryLevel(const SYSTEM_POWER_STATUS& status) {
    batteryLevelPercent = (status.BatteryLifePercent == 255)
        ? -1
        : status.BatteryLifePercent;
}

void Lab1::updateBatteryStatus(const SYSTEM_POWER_STATUS& status) {
    isCharging = false;
    switch (status.BatteryFlag) {
    case 128: batteryStatusStr = "No battery"; break;
    case 8:   batteryStatusStr = "Charging"; isCharging = true; break;
    case 2:   batteryStatusStr = "Low (33%>)"; break;
    case 1:   batteryStatusStr = "High (66%<)"; break;
    case 4:   batteryStatusStr = "Critical (5%>)"; break;
    case 0:   batteryStatusStr = "Normal (34-65%)"; break;
    default:  batteryStatusStr = "Unknown"; break;
    }
}

void Lab1::updateBatteryTimes(const SYSTEM_POWER_STATUS& status) {
    batteryRunTimeMinutes =
        (status.BatteryLifeTime == (DWORD)-1)
        ? -1
        : status.BatteryLifeTime / 60;

    if (status.BatteryLifeTime != (DWORD)-1 &&
        status.BatteryLifePercent != 255 &&
        status.BatteryLifePercent > 0) {
        batteryFullLifeTimeMinutes =
            (status.BatteryLifeTime / 60) * 100 / status.BatteryLifePercent;
    }
    else {
        batteryFullLifeTimeMinutes = -1;
    }
}


void Lab1::updatePowerInfo() {
    SYSTEM_POWER_STATUS status;
    bool wasCharging = isCharging;

    if (GetSystemPowerStatus(&status)) {
        updatePowerMode(status);
        updatePowerSource(status);
        updateBatteryLevel(status);
        updateBatteryStatus(status);
        updateBatteryTimes(status);


    }
}



void Lab1::handleEvents(bool& running) {
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

        sleepButton.handleEvent(event, window);
        hibernateButton.handleEvent(event, window);

        if (event.type == sf::Event::MouseMoved) {
            currentMousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            
                mouseOverSleepButton = sleepButton.isMouseOver(currentMousePos);
                mouseOverHibernateButton = hibernateButton.isMouseOver(currentMousePos);

                if (mouseOverSleepButton) {
                    character.setButtonTargetLeft(sleepButton.getPosition(), sleepButton.getSize());
                    character.setPointing(true);
                }
                else if (mouseOverHibernateButton) {
                    character.setButtonTargetLeft(hibernateButton.getPosition(), hibernateButton.getSize());
                    character.setPointing(true);
                }
                else {
                    character.setPointing(false);
                    character.moveToMouse(currentMousePos);
                }
            }
        

    }
}

void Lab1::render() {
    window.clear(sf::Color(40, 40, 40));
    if (backgroundTexture.getSize().x > 0) {
        window.draw(backgroundSprite);
    }

    character.draw(window);

    sf::RectangleShape textBackground(sf::Vector2f(currentWindowSize.x * 0.45f, currentWindowSize.y * 0.45f));
    textBackground.setPosition(currentWindowSize.x * 0.03f, currentWindowSize.y * 0.03f);
    textBackground.setFillColor(sf::Color(0, 0, 0, 180)); 
    textBackground.setOutlineColor(sf::Color::White);
    textBackground.setOutlineThickness(1.f);
    window.draw(textBackground);

    unsigned int titleSize = static_cast<unsigned int>(currentWindowSize.y * 0.045f);
    unsigned int infoSize = static_cast<unsigned int>(currentWindowSize.y * 0.025f);

    float titleX = currentWindowSize.x * 0.05f;
    float titleY = currentWindowSize.y * 0.05f;
    float infoX = currentWindowSize.x * 0.05f;
    float infoY = currentWindowSize.y * 0.15f;

    sf::Text title;
    title.setFont(font);
    title.setString("Laboratory Work 1 - Battery");
    title.setCharacterSize(titleSize);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition(titleX, titleY);
    window.draw(title);

    sf::Text info;
    info.setFont(font);
    info.setCharacterSize(infoSize);
    info.setFillColor(sf::Color::White);
    info.setPosition(infoX, infoY);

    std::string infoStr = "Power Source: " + powerSourceStr + "\n" +
        "Battery Type: " + batteryTypeStr + "\n" +
        "Battery Status: " + batteryStatusStr + "\n" +
        "Battery Level: " + (batteryLevelPercent == -1 ? "Unknown" : std::to_string(batteryLevelPercent) + "%") + "\n" +
        powerModeStr + "\n" +
        "Battery Runtime: " + (batteryRunTimeMinutes == -1 ? "Unknown" : std::to_string(batteryRunTimeMinutes) + " min") + "\n" +
        "Battery Full Life: " + (batteryFullLifeTimeMinutes == -1 ? "Unknown" : std::to_string(batteryFullLifeTimeMinutes) + " min");

    info.setString(infoStr);
    window.draw(info);

    // Кнопки
    sleepButton.draw(window);
    hibernateButton.draw(window);

    window.display();
}

void Lab1::run() {
    sf::Clock clock;
    bool running = true;

    batteryTypeStr = GetBatteryType();
    loadBackground(); 
    updateLayout(); 
    initializeButtons();

    while (running && window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleEvents(running);
        updatePowerInfo();
        updateCharacter(deltaTime); 

        sleepButton.update(deltaTime);
        hibernateButton.update(deltaTime);

        render();
    }
}