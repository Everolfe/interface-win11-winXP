#include <windows.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

using namespace std;


typedef bool (__stdcall *pInitializeWinIo)();
typedef void (__stdcall *pShutdownWinIo)();
//WORD - адрес PDWORD - куда записывается, BYTE - размер данных для чтения 
typedef bool (__stdcall *pGetPortVal)(WORD, PDWORD, BYTE);
//WORD - адрес PDWORD - что записывается, BYTE - размер данных для записи 
typedef bool (__stdcall *pSetPortVal)(WORD, DWORD, BYTE);

struct SATAPort {
    WORD commandBase;
    WORD controlBase;
    string name;
};


SATAPort sataPorts[] = {
    {0x1F0, 0x3F6, "SATA Port 0"},
    {0x170, 0x376, "SATA Port 1"},
};


pInitializeWinIo InitializeWinIo = NULL;
pShutdownWinIo ShutdownWinIo = NULL;
pGetPortVal GetPortVal = NULL;
pSetPortVal SetPortVal = NULL;

string intToString(int value) {
    stringstream ss;
    ss << value;
    return ss.str();
}

string wordToString(WORD value) {
    stringstream ss;
    ss << value;
    return ss.str();
}

string hexToString(WORD value) {
    stringstream ss;
    ss << hex << uppercase << value;
    return ss.str();
}

struct DiskSpaceInfo {
    unsigned long long totalBytes;
    unsigned long long freeBytes;
    unsigned long long usedBytes;
    bool isValid;
    string fileSystem;
    string volumeName;
};

vector<pair<string, DiskSpaceInfo> > getAllDrivesSpaceInfo() {
    vector<pair<string, DiskSpaceInfo> > drivesInfo;
    
    DWORD drives = GetLogicalDrives();
    char driveLetter[] = "A:\\";
    
    for (int i = 0; i < 26; i++) {
        if (drives & (1 << i)) {
            driveLetter[0] = 'A' + i;
            DWORD type = GetDriveTypeA(driveLetter);
            
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                DiskSpaceInfo info;
                ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
                
                if (GetDiskFreeSpaceExA(driveLetter, &freeBytesAvailable, 
                                      &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
                    info.totalBytes = totalNumberOfBytes.QuadPart;
                    info.freeBytes = totalNumberOfFreeBytes.QuadPart;
                    info.usedBytes = info.totalBytes - info.freeBytes;
                    info.isValid = true;
                    
                    char fileSystemName[32];
                    char volumeName[128];
                    DWORD serialNumber, maxComponentLength, fileSystemFlags;
                    
                    if (GetVolumeInformationA(driveLetter, volumeName, sizeof(volumeName),
                                            &serialNumber, &maxComponentLength, &fileSystemFlags,
                                            fileSystemName, sizeof(fileSystemName))) {
                        info.fileSystem = fileSystemName;
                        info.volumeName = volumeName;
                    } else {
                        info.fileSystem = "Unknown";
                        info.volumeName = "Unknown";
                    }
                } else {
                    info.isValid = false;
                }
                
                drivesInfo.push_back(make_pair(string(driveLetter), info));
            }
        }
    }
    
    return drivesInfo;
}

bool waitDiskReady(WORD basePort, DWORD timeoutMs = 2000) {
    DWORD status;
    for (DWORD i = 0; i < timeoutMs; i++) {
        if (!GetPortVal(basePort + 7, &status, 1)) { // 0x1F7 SR - получение состояния в status
            if (!GetPortVal(basePort + 0x206, &status, 1))// 0x3F6 AS - альтернативный адрес регистра состояния - получинени состояния в status
				return false;
        }

        if (status & 0x80) { // 7 бит BSY
            Sleep(1);
            continue;
        }
        
        if (!(status & 0x80)) {
            if (status & 0x08) return true; // бит 3(DRQ) равен 1 и готов к обмену
            if (!(status & 0x01)) return true; // бит 0 равен 1 (ошибка)
        }
        Sleep(1);
    }
    return false;
}



bool sendIdentifyCommand(WORD basePort, BYTE drive) {
	// basePort = 0x1F0, зависит от канала
	// drive - записываемые биты 
	// Выбор устройства (drive = 11100000 (Ведущее)или 11110000(Ведомое)
    if (!SetPortVal(basePort + 6, drive, 1)) {// 0x1F6 - DH
//		Bit 7: 1 = резервирован, устарел
//		Bit 6: 1 = адресация  LBA (0 - CHS номер головки)
//		Bit 5: 1 = резервирован, устарел
//		Bit 4: 0 = Master, 1 = Slave
//		Bit 3-0: старшие LBA биты  
            return false;
    }

	//обнуление регистров
    SetPortVal(basePort + 2, 0, 1);// 0x1F2 регистр SC - Количество секторов должно обнулится по адресу
	SetPortVal(basePort + 3, 0, 1); //0x1F3 SN - LBA [7:0]
    SetPortVal(basePort + 4, 0, 1); //0x1F4 CL - LBA [15:8]
	SetPortVal(basePort + 5, 0, 1); //0x1F5 CH - LBA [23:16]

    if (!waitDiskReady(basePort, 1000)) {
        SetPortVal(basePort + 0x206, 0x04, 1); //0x3F6 DC - (управление устройством)  Установка бита SRST (00000010)
        Sleep(100);
        SetPortVal(basePort + 0x206, 0x00, 1); //0x3F6 DC - Сброс бита SRST
        Sleep(100);
        
        if (!waitDiskReady(basePort, 1000))
            return false;
    }
	// 0xEC - IDENTIFY DEVICE
    if (!SetPortVal(basePort + 7, 0xEC, 1)) { // 0x1F7 CR - отправляет команду 
            return false;
    }

    DWORD status;
    for (int i = 0; i < 10000; i++) {
        if (!GetPortVal(basePort + 7, &status, 1)) { // 0x1F7 SR - Получение текущего статуса устройства в status

        }
        
        if (status & 0x08) return true; // 3 (DRQ) бит равен 1 и готово к передаче
        if (status & 0x01) { // бит 0 равен 1 и сообщение об ошибке
            DWORD error;
            GetPortVal(basePort + 1, &error, 1); // 0x1F1 ER - получение данных из регистра ошибок в error
            return false;
        }
        Sleep(1);
    }
    return false;
}

// ====== Чтение данных IDENTIFY ======
bool readIdentifyData(WORD basePort, WORD* buffer) {
    for (int i = 0; i < 256; i++) {
        DWORD data;
        if (!GetPortVal(basePort, &data, 2)) { // 0x1F0 DR - получение данных в data размером 2 байта
            if (!GetPortVal(basePort, &data, 4)) //4 байта
                return false;
        }
        buffer[i] = (WORD)(data & 0xFFFF);
        
        if (i % 16 == 0) Sleep(1);
    }
    return true;
}

// ====== Чтение строки из буфера ======
string readStringFromBuffer(WORD* buffer, int startWord, int length) {
    string result;
    for (int i = startWord; i < startWord + length; i++) {
        char ch1 = (char)(buffer[i] & 0xFF);
        char ch2 = (char)((buffer[i] >> 8) & 0xFF);
        
        if (ch1 != 0 && ch1 != ' ' && ch1 != 0xFF) result += ch1;
        if (ch2 != 0 && ch2 != ' ' && ch2 != 0xFF) result += ch2;
    }
    
    size_t start = result.find_first_not_of(' ');
    if (start == string::npos) return "";
    size_t end = result.find_last_not_of(' ');
    return result.substr(start, end - start + 1);
}

// ====== Определение типа диска ======
string getDiskType(WORD* buffer) {
    if (buffer[0] == 0xFFFF || buffer[0] == 0x0000) 
        return "Unknown";
    
    WORD nominalMediaRotationRate = buffer[217];
    if (nominalMediaRotationRate == 0x0001) 
        return "SSD";
    
    string model = readStringFromBuffer(buffer, 27, 20);
    string modelUpper = model;
    for (size_t i = 0; i < modelUpper.length(); i++)
        modelUpper[i] = toupper(modelUpper[i]);
    
    if (modelUpper.find("SSD") != string::npos) return "SSD";
    if (modelUpper.find("SOLID") != string::npos) return "SSD";
    
    return "HDD";
}

// ====== Чтение общей емкости ======
unsigned __int64 getTotalCapacity(WORD* buffer) {
    unsigned __int64 sectors48 = 0;
    sectors48 |= ((unsigned __int64)buffer[100]) << 0;
    sectors48 |= ((unsigned __int64)buffer[101]) << 16;
    sectors48 |= ((unsigned __int64)buffer[102]) << 32;
    sectors48 |= ((unsigned __int64)buffer[103]) << 48;
    
    if (sectors48 > 0 && sectors48 != 0xFFFFFFFFFFFFFFFF) {
        return sectors48 * 512;
    }
    
    return 0;
}

// ====== Форматирование размера ======
string formatSize(unsigned __int64 bytes) {
    if (bytes == 0) return "Unknown";
    
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        unitIndex++;
    }
    
    char buffer[32];
    sprintf_s(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return string(buffer);
}

// ====== Получение поддерживаемых режимов ======
vector<string> getSupportedModes(WORD* buffer) {
    vector<string> modes;
    
    WORD capabilities = buffer[49];
    if (capabilities & 0x0100) modes.push_back("LBA");
    if (capabilities & 0x0200) modes.push_back("DMA");
    
    WORD features = buffer[53];
    if (features & 0x0002) modes.push_back("SMART");
    
    WORD udma = buffer[88];
    if (udma & 0x00FF) {
        modes.push_back("UDMA");
    }
    
    WORD pio_modes = buffer[64];
    if (pio_modes & 0x0001) modes.push_back("PIO0");
    if (pio_modes & 0x0002) modes.push_back("PIO1");
    if (pio_modes & 0x0004) modes.push_back("PIO2");
    if (pio_modes & 0x0008) modes.push_back("PIO3");
    if (pio_modes & 0x0010) modes.push_back("PIO4");
    
    WORD majorVersion = buffer[80];
    if (majorVersion & 0x2000) modes.push_back("ATA6");
    if (majorVersion & 0x1000) modes.push_back("ATA5");
    if (majorVersion & 0x0800) modes.push_back("ATA4");
    if (majorVersion & 0x0400) modes.push_back("ATA3");
    if (majorVersion & 0x0200) modes.push_back("ATA2");
    if (majorVersion & 0x0100) modes.push_back("ATA1");
    
    if (modes.empty()) {
        modes.push_back("PIO");
    }
    
    return modes;
}

// ====== Запись информации в файл ======
void writeToFile(const string& filename, const string& content) {
    ofstream file(filename.c_str(), ios::app);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}

// ====== Получение информации о диске ======
bool getDiskInfo(WORD basePort, BYTE drive, const string& interfaceType) {
    stringstream portInfo;
    portInfo << "Scanning " << interfaceType << " (Port 0x" << hex << uppercase << basePort << ")... ";
    cout << portInfo.str();
    
    if (!sendIdentifyCommand(basePort, drive)) {
        cout << "No response" << endl;
        return false;
    }
    
    WORD identifyData[256];
    if (!readIdentifyData(basePort, identifyData)) {
        cout << "Read failed" << endl;
        return false;
    }
    
    if (identifyData[0] == 0x0000 || identifyData[0] == 0xFFFF) {
        cout << "Invalid data" << endl;
        return false;
    }
    
    string model = readStringFromBuffer(identifyData, 27, 20);
    string serial = readStringFromBuffer(identifyData, 10, 10);
    string firmware = readStringFromBuffer(identifyData, 23, 4);
    
    if (model.empty()) {
        cout << "Empty model" << endl;
        return false;
    }
    
    string diskType = getDiskType(identifyData);
    unsigned __int64 totalSize = getTotalCapacity(identifyData);
    vector<string> supportedModes = getSupportedModes(identifyData);
    
    cout << "Found " << diskType << endl;
    cout << "==========================================" << endl;
    cout << "Model: " << model << endl;
    cout << "Serial Number: " << serial << endl;
    cout << "Firmware Version: " << firmware << endl;
    cout << "Disk Type: " << diskType << endl;
    cout << "Interface: " << interfaceType << endl;
    cout << "Total Physical Size: " << formatSize(totalSize) << endl;
    
    cout << "Supported Modes: ";
    for (size_t i = 0; i < supportedModes.size(); i++) {
        cout << supportedModes[i];
        if (i < supportedModes.size() - 1) cout << ", ";
    }
    cout << endl << endl;
    
    // --- Новый код: получение логического диска и информации о свободном месте ---
    vector<pair<string, DiskSpaceInfo>> drives = getAllDrivesSpaceInfo();
bool matchedDrive = false;

for (vector<pair<string, DiskSpaceInfo>>::const_iterator it = drives.begin(); it != drives.end(); ++it)
{
    const pair<string, DiskSpaceInfo>& d = *it;

    if (d.second.isValid) {
        unsigned __int64 diff = (totalSize > d.second.totalBytes)
                                    ? totalSize - d.second.totalBytes
                                    : d.second.totalBytes - totalSize;

        if (diff < (1ULL << 30)) {
            cout << "Logical Drive: " << d.first << endl;
            cout << "  Volume Name: " << d.second.volumeName << endl;
            cout << "  File System: " << d.second.fileSystem << endl;
            cout << "  Used Space: " << formatSize(d.second.usedBytes) << endl;
            cout << "  Free Space: " << formatSize(d.second.freeBytes) << endl;
            cout << endl;
            matchedDrive = true;

            stringstream fileContent;
            fileContent << "Model: " << model << endl
                        << "Serial: " << serial << endl
                        << "Firmware: " << firmware << endl
                        << "Type: " << diskType << endl
                        << "Interface: " << interfaceType << endl
                        << "Total Physical Size: " << formatSize(totalSize) << endl
                        << "Used Space: " << formatSize(d.second.usedBytes) << endl
                        << "Free Space: " << formatSize(d.second.freeBytes) << endl
                        << "Supported Modes: ";
            for (size_t i = 0; i < supportedModes.size(); i++) {
                fileContent << supportedModes[i];
                if (i < supportedModes.size() - 1) fileContent << ", ";
            }
            fileContent << endl;

            writeToFile("\\\\?\\UNC\\vmware-host\\Shared Folders\\winda\\pciTxt\\discks.txt", fileContent.str());
            break;
        }
    }
}

    
    // Если не удалось сопоставить логический диск, всё равно записываем базовую информацию
    if (!matchedDrive) {
        stringstream fileContent;
        fileContent << "Model: " << model << endl
                    << "Serial: " << serial << endl
                    << "Firmware: " << firmware << endl
                    << "Type: " << diskType << endl
                    << "Interface: " << interfaceType << endl
                    << "Total Physical Size: " << formatSize(totalSize) << endl
                    << "Supported Modes: ";
        for (size_t i = 0; i < supportedModes.size(); i++) {
            fileContent << supportedModes[i];
            if (i < supportedModes.size() - 1) fileContent << ", ";
        }
        fileContent << endl << "(No matching logical drive found)" << endl
                    << "---" << endl;
        
        writeToFile("\\\\?\\UNC\\vmware-host\\Shared Folders\\winda\\pciTxt\\discks.txt", fileContent.str());
    }
    
    return true;
}


// ====== Вывод подробной информации о дисковом пространстве ======
void printDiskSpaceInfo() {
    cout << "=== LOGICAL DRIVES INFORMATION ===" << endl;
    vector<pair<string, DiskSpaceInfo> > drivesSpaceInfo = getAllDrivesSpaceInfo();
    
    if (!drivesSpaceInfo.empty()) {
        for (vector<pair<string, DiskSpaceInfo> >::const_iterator it = drivesSpaceInfo.begin();
             it != drivesSpaceInfo.end(); ++it) {
            if (it->second.isValid) {
                cout << "Drive " << it->first << ":" << endl;
                cout << "  Volume Name: " << it->second.volumeName << endl;
                cout << "  File System: " << it->second.fileSystem << endl;
                cout << "  Total Size: " << formatSize(it->second.totalBytes) << endl;
                cout << "  Used Space: " << formatSize(it->second.usedBytes) << endl;
                cout << "  Free Space: " << formatSize(it->second.freeBytes) << endl;
                
                if (it->second.totalBytes > 0) {
                    double usagePercent = (double)it->second.usedBytes / it->second.totalBytes * 100.0;
                    cout << "  Usage: " << fixed << setprecision(1) << usagePercent << "%" << endl;
                }
                cout << endl;
            }
        }
        
        // Суммарная информация
        cout << "=== SUMMARY ===" << endl;
        unsigned __int64 totalAllDrives = 0;
        unsigned __int64 usedAllDrives = 0;
        unsigned __int64 freeAllDrives = 0;
        
        for (vector<pair<string, DiskSpaceInfo> >::const_iterator it = drivesSpaceInfo.begin();
             it != drivesSpaceInfo.end(); ++it) {
            if (it->second.isValid) {
                totalAllDrives += it->second.totalBytes;
                usedAllDrives += it->second.usedBytes;
                freeAllDrives += it->second.freeBytes;
            }
        }
        
        cout << "Total Drives: " << drivesSpaceInfo.size() << endl;
        cout << "Combined Total: " << formatSize(totalAllDrives) << endl;
        cout << "Combined Used: " << formatSize(usedAllDrives) << endl;
        cout << "Combined Free: " << formatSize(freeAllDrives) << endl;
        
    } else {
        cout << "No drive space information available" << endl;
    }
    cout << endl;
}

// ====== Сканирование всех возможных портов ======
void scanAllPorts() {
    int diskCount = 0;
    
    cout << "SCANNING PHYSICAL DISKS (IDE/SATA PORTS)..." << endl;
    cout << "============================================" << endl << endl;
    
    // Сначала выводим общую информацию о логических дисках ОДИН РАЗ
    printDiskSpaceInfo();
    
    cout << "SCANNING PHYSICAL DISK PORTS..." << endl;
    cout << "===============================" << endl << endl;
    
    // Затем сканируем физические диски
	// 0xE0 11100000 - ведущее
	// 0xF0 11110000 - ведомое
	for (int i = 0; i < 2; i++) {
        if (getDiskInfo(sataPorts[i].commandBase, 0xE0, sataPorts[i].name + " Master")) 
            diskCount++;
        if (getDiskInfo(sataPorts[i].commandBase, 0xF0, sataPorts[i].name + " Slave")) 
            diskCount++;
    }
    
    
    cout << "=== SCAN COMPLETE ===" << endl;
    cout << "Total PHYSICAL disks found: " << diskCount << endl;
    cout << "Total LOGICAL drives: " << getAllDrivesSpaceInfo().size() << endl;
}

// ====== Инициализация WinIo ======
bool initializeWinIo() {
    HMODULE hWinIo = LoadLibraryA("WinIo32.dll");
    if (!hWinIo) {
        hWinIo = LoadLibraryA("C:\\WinIo\\Binaries\\WinIo32.dll");
    }
    
    if (!hWinIo) {
        cout << "ERROR: Cannot load WinIo32.dll" << endl;
        return false;
    }
    
    InitializeWinIo = (pInitializeWinIo)GetProcAddress(hWinIo, "InitializeWinIo");
    ShutdownWinIo = (pShutdownWinIo)GetProcAddress(hWinIo, "ShutdownWinIo");
    GetPortVal = (pGetPortVal)GetProcAddress(hWinIo, "GetPortVal");
    SetPortVal = (pSetPortVal)GetProcAddress(hWinIo, "SetPortVal");
    
    if (!InitializeWinIo || !ShutdownWinIo || !GetPortVal || !SetPortVal) {
        cout << "ERROR: Cannot get WinIo function addresses" << endl;
        FreeLibrary(hWinIo);
        return false;
    }
    
    if (!InitializeWinIo()) {
        cout << "ERROR: Cannot initialize WinIo" << endl;
        FreeLibrary(hWinIo);
        return false;
    }
    
    return true;
}

// ====== Главная функция ======
int main() {
    cout << "VMware Disk Information Scanner" << endl;
    cout << "Using WinIo driver for direct port access" << endl;
    cout << "==========================================" << endl << endl;
    
    // Удаляем старый файл перед началом
    remove("\\\\?\\UNC\\vmware-host\\Shared Folders\\winda\\pciTxt\\discks.txt");
    
    if (!initializeWinIo()) {
        system("pause");
        return 1;
    }
    
    cout << "WinIo initialized successfully" << endl << endl;
    
    scanAllPorts();
    
    //ShutdownWinIo();
    cout << "Scan completed. Results saved to disk_info.txt" << endl;
    system("pause");
    return 0;
}