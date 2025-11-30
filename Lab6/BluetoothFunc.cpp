#include "BluetoothFunc.hpp"
std::string CoDMajorToString(DWORD classOfDevice) {
    int major = (classOfDevice & 0x1F00) >> 8;
    switch (major) {
    case 0x01: return "Computer";
    case 0x02: return "Phone";
    case 0x03: return "LAN/Network Access Point";
    case 0x04: return "Audio/Video";
    case 0x05: return "Peripheral";
    default: return "Unknown";
    }
}

std::string BtAddrToString(ULONGLONG btAddr) {
    char buf[18];
    sprintf_s(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
        (btAddr >> 40) & 0xFF,
        (btAddr >> 32) & 0xFF,
        (btAddr >> 24) & 0xFF,
        (btAddr >> 16) & 0xFF,
        (btAddr >> 8) & 0xFF,
        btAddr & 0xFF);
    return std::string(buf);
}

TargetType DetectTargetType(const std::wstring& deviceName, DWORD classOfDevice, const std::string& deviceClass) {
    if (deviceClass == "Phone") {
        return ANDROID;
    }
    else if (deviceClass == "Computer") {
        return WINDOWS;
    }

    std::wstring lowerName = deviceName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    if (lowerName.find(L"android") != std::wstring::npos ||
        lowerName.find(L"samsung") != std::wstring::npos ||
        lowerName.find(L"xiaomi") != std::wstring::npos ||
        lowerName.find(L"huawei") != std::wstring::npos ||
        lowerName.find(L"oneplus") != std::wstring::npos ||
        lowerName.find(L"google") != std::wstring::npos ||
        lowerName.find(L"pixel") != std::wstring::npos ||
        lowerName.find(L"phone") != std::wstring::npos ||
        lowerName.find(L"mobile") != std::wstring::npos) {
        return ANDROID;
    }

    if (lowerName.find(L"pc") != std::wstring::npos ||
        lowerName.find(L"laptop") != std::wstring::npos ||
        lowerName.find(L"notebook") != std::wstring::npos ||
        lowerName.find(L"desktop") != std::wstring::npos ||
        lowerName.find(L"windows") != std::wstring::npos ||
        lowerName.find(L"surface") != std::wstring::npos ||
        lowerName.find(L"thinkpad") != std::wstring::npos) {
        return WINDOWS;
    }

    return UNKNOWN;
}

std::string TargetTypeToString(TargetType type) {
    switch (type) {
    case ANDROID: return "Android";
    case WINDOWS: return "Windows";
    default: return "Unknown";
    }
}

void ListBluetoothDevices(std::vector<BluetoothDevice>& devices) {
    devices.clear();

    //параметры поиска устройств bluetooth
    BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = { 0 };
    searchParams.dwSize = sizeof(searchParams);
    searchParams.fReturnAuthenticated = TRUE;
    searchParams.fReturnRemembered = TRUE;
    searchParams.fReturnUnknown = FALSE;
    searchParams.fReturnConnected = TRUE;
    searchParams.fIssueInquiry = TRUE;
    searchParams.cTimeoutMultiplier = 4;

        
    BLUETOOTH_DEVICE_INFO deviceInfo = { 0 };
    deviceInfo.dwSize = sizeof(deviceInfo);

    HBLUETOOTH_DEVICE_FIND hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
    if (!hFind) {
        DWORD err = GetLastError();
        return;
    }

    int idx = 0;
    do {
        BluetoothDevice device;
        device.name = deviceInfo.szName;
        device.addressStr = BtAddrToString(deviceInfo.Address.ullLong);
        device.deviceClass = CoDMajorToString(deviceInfo.ulClassofDevice);
        device.classOfDevice = deviceInfo.ulClassofDevice;
        device.targetType = DetectTargetType(deviceInfo.szName, deviceInfo.ulClassofDevice, device.deviceClass);

        device.address.addressFamily = AF_BTH;
        device.address.btAddr = deviceInfo.Address.ullLong;

        devices.push_back(device);
        idx++;
        deviceInfo.dwSize = sizeof(deviceInfo);
    } while (BluetoothFindNextDevice(hFind, &deviceInfo) == TRUE);

    BluetoothFindDeviceClose(hFind);
}

bool GetRfcommChannel(const SOCKADDR_BTH& deviceAddr, UCHAR& outChannel) {
    //структура запроса
    WSAQUERYSET querySet = { 0 };
    querySet.dwNameSpace = NS_BTH;
    querySet.lpServiceClassId = (LPGUID)&SPP_UUID;
    querySet.dwSize = sizeof(querySet);
    querySet.dwNumberOfCsAddrs = 0;
    querySet.lpszContext = NULL;
    querySet.lpBlob = NULL;

    HANDLE hLookup = NULL;
    DWORD flags = LUP_FLUSHCACHE | LUP_RETURN_ADDR | LUP_RETURN_NAME | LUP_RETURN_BLOB;

    if (WSALookupServiceBegin(&querySet, flags, &hLookup) != 0) {
        std::cerr << "WSALookupServiceBegin failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    char buffer[4096];
    DWORD bufLen = sizeof(buffer);
    WSAQUERYSET* pResults = reinterpret_cast<WSAQUERYSET*>(buffer);
    bool found = false;

    while (WSALookupServiceNext(hLookup, flags, &bufLen, pResults) == 0) {
        if (pResults->lpcsaBuffer && pResults->lpcsaBuffer->RemoteAddr.lpSockaddr) {
            SOCKADDR_BTH* addr = reinterpret_cast<SOCKADDR_BTH*>(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr);
            if (addr->btAddr == deviceAddr.btAddr) {
                outChannel = addr->port;
                std::cout << "Found RFCOMM channel: " << (int)outChannel << std::endl;
                found = true;
                break;
            }
        }
        bufLen = sizeof(buffer);
    }

    WSALookupServiceEnd(hLookup);

    if (!found) {
        std::cerr << "RFCOMM channel not found via SDP" << std::endl;
    }

    return found;
}

bool ConnectToDevice(const SOCKADDR_BTH& deviceAddr, TargetType target, SOCKET& outSocket) {
    //инициализаци€ winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    outSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (outSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Err=" << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // ”станавливаем таймауты
    DWORD timeout = 10000; // 10 секунд
    setsockopt(outSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(outSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    SOCKADDR_BTH remoteAddr = deviceAddr;
    remoteAddr.serviceClassId = SPP_UUID;

    // —начала пытаемс€ найти правильный RFCOMM канал через SDP
    UCHAR rfcommChannel;
    if (GetRfcommChannel(deviceAddr, rfcommChannel)) {
        std::cout << "Using SDP-discovered channel: " << (int)rfcommChannel << std::endl;
        remoteAddr.port = rfcommChannel;

        if (connect(outSocket, (SOCKADDR*)&remoteAddr, sizeof(remoteAddr)) == 0) {
            std::cout << "Connected successfully via SDP channel!" << std::endl;
            return true;
        }
        else {
            std::cout << "SDP channel failed, trying other channels..." << std::endl;
        }
    }

    // ≈сли SDP не сработал, перебираем стандартные порты
    int portsToTry[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                        21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };

    int startPort = (target == ANDROID) ? 1 : 1;

    std::cout << "Trying to connect through multiple RFCOMM channels..." << std::endl;

    for (int i = startPort; i < sizeof(portsToTry) / sizeof(portsToTry[0]); i++) {
        remoteAddr.port = portsToTry[i];

        std::cout << "Trying channel " << portsToTry[i] << "... ";

        if (connect(outSocket, (SOCKADDR*)&remoteAddr, sizeof(remoteAddr)) == 0) {
            std::cout << "SUCCESS!" << std::endl;
            std::cout << "Connected via RFCOMM channel: " << portsToTry[i] << std::endl;
            return true;
        }
        else {
            DWORD error = WSAGetLastError();
            std::cout << "Failed (Error: " << error << ")" << std::endl;

            closesocket(outSocket);
            outSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
            if (outSocket == INVALID_SOCKET) {
                std::cerr << "Failed to recreate socket. Err=" << WSAGetLastError() << std::endl;
                WSACleanup();
                return false;
            }

            setsockopt(outSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
            setsockopt(outSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "Failed to connect to any RFCOMM channel!" << std::endl;
    closesocket(outSocket);
    WSACleanup();
    return false;
}

bool ConnectToAndroid(const SOCKADDR_BTH& deviceAddr, SOCKET& outSocket) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    outSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (outSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket. Err=" << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    SOCKADDR_BTH remoteAddr = deviceAddr;
    remoteAddr.serviceClassId = SPP_UUID;
    remoteAddr.addressFamily = AF_BTH;
    remoteAddr.port = 0;

    if (connect(outSocket, (SOCKADDR*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
        DWORD err = WSAGetLastError();
        std::cerr << "Connect failed: " << err << std::endl;
        closesocket(outSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Connected to Android successfully!" << std::endl;
    return true;
}

void SendFile(const std::string& filePath, const BluetoothDevice& device) {
    SOCKET btSocket;
    bool connected = (device.targetType == ANDROID) ? ConnectToAndroid(device.address, btSocket)
        : ConnectToDevice(device.address, device.targetType, btSocket);
    if (!connected) return;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Cannot open file\n";
        closesocket(btSocket);
        WSACleanup();
        return;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::cout << "File size: " << fileSize << " bytes\n";

    const size_t bufferSize = 100 * 1024;
    char buffer[bufferSize];

    std::streamsize totalSentBytes = 0;

    while (file.good() && totalSentBytes < fileSize) {
        file.read(buffer, bufferSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        std::streamsize totalSent = 0;
        while (totalSent < bytesRead) {
            int sent = send(btSocket, buffer + totalSent, static_cast<int>(bytesRead - totalSent), 0);
            if (sent == SOCKET_ERROR) {
                std::cerr << "Send failed. Err=" << WSAGetLastError() << "\n";
                file.close();
                closesocket(btSocket);
                WSACleanup();
                return;
            }
            totalSent += sent;
            totalSentBytes += sent;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    shutdown(btSocket, SD_SEND);
    std::this_thread::sleep_for(std::chrono::seconds(20));
    file.close();

    closesocket(btSocket);
    WSACleanup();

    std::cout << "File sent successfully! Total bytes sent: " << totalSentBytes << "\n";
}

void ReceiveFile(SOCKET clientSocket) {
    char buffer[100 * 1024];
    int bytesReceived;
    std::ofstream file("received_file.mp3", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to create file." << std::endl;
        return;
    }

    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, bytesReceived);
        std::cout << "Received " << bytesReceived << " bytes\n";
    }
    if (bytesReceived == SOCKET_ERROR) {
        std::cerr << "Recv failed. Err=" << WSAGetLastError() << std::endl;
    }
    file.close();

    std::cout << "Auto-playback..." << std::endl;
    ShellExecuteA(NULL, "open", "received_file.mp3", NULL, NULL, SW_SHOWNORMAL);
}

void RunServer() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    SOCKET serverSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "socket failed. Err=" << WSAGetLastError() << std::endl;
        WSACleanup();
        return;
    }

    int portsToTry[] = { 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31 };
    bool bindSuccess = false;
    SOCKADDR_BTH serverAddr = { 0 };

    for (int port : portsToTry) {
        serverAddr.addressFamily = AF_BTH;
        serverAddr.port = port;
        serverAddr.serviceClassId = SPP_UUID;

        if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == 0) {
            std::cout << "Successfully bound to RFCOMM channel: " << port << std::endl;
            bindSuccess = true;
            break;
        }
        else {
            std::cout << "Failed to bind to channel " << port << ", trying next..." << std::endl;
        }
    }

    if (!bindSuccess) {
        std::cerr << "Failed to bind to any RFCOMM channel. Err=" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed. Err=" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    std::cout << "Server is listening for incoming connections..." << std::endl;
    std::cout << "Make sure Bluetooth is visible and discoverable!" << std::endl;

    SOCKET clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed. Err=" << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    std::cout << "Connection accepted!" << std::endl;
    ReceiveFile(clientSocket);

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}
