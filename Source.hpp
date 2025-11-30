#pragma once

// ==================== СИСТЕМНЫЕ БИБЛИОТЕКИ ====================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCKAPI_

// Сначала стандартные библиотеки C++
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>
#include <algorithm>
#include <iomanip>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>
#include <filesystem>
#include <utility>

// Сетевые/Bluetooth
#include <winsock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

// Затем Windows API (до ATL)
#include <shellapi.h>
#include <windows.h>
#include <winbase.h>
#include <winioctl.h>
#include <setupapi.h>
#include <devguid.h>
#include <Poclass.h>
#include <powrprof.h>
#include <Cfgmgr32.h>
#include <Dbt.h>
#include <Usbiodef.h>
#include <initguid.h>

#include <commdlg.h> // Для OpenFileName
#include <shlobj.h>  // Для SHBrowseForFolder
#include <commdlg.h>

// Определяем недостающие GUID перед включением ATL
#ifndef GUID_NULL
DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
#endif

#ifndef CLSID_StdGlobalInterfaceTable
DEFINE_GUID(CLSID_StdGlobalInterfaceTable, 0x00000323, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
#endif

// Теперь ATL и DirectShow
#include <atlbase.h>
#include <atlcom.h>
#include <dshow.h>
#include <comdef.h>

// SFML
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

// OpenCV (опционально)

#include <opencv2/opencv.hpp>


// Прагмы для линковки библиотек
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Bthprops.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "comdlg32.lib")