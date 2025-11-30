#include <windows.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

typedef bool (__stdcall *pInitializeWinIo)();
typedef void (__stdcall *pShutdownWinIo)();
typedef bool (__stdcall *pGetPortVal)(WORD, PDWORD, BYTE);
typedef bool (__stdcall *pSetPortVal)(WORD, DWORD, BYTE);

DWORD pciConfigReadDword(pSetPortVal SetPortVal, pGetPortVal GetPortVal,
                         BYTE bus, BYTE device, BYTE function, BYTE offset)
{
    DWORD address = ((DWORD)1 << 31) | ((DWORD)bus << 16) |
                    ((DWORD)device << 11) | ((DWORD)function << 8) | (offset & 0xFC);

    if (!SetPortVal(0xCF8, address, 4)) return 0xFFFFFFFF;

    DWORD data = 0xFFFFFFFF;
    if (!GetPortVal(0xCFC, &data, 4)) return 0xFFFFFFFF;

    return data;
}

long int count = 0;
int main()
{
    const char* outputPath = "\\\\?\\UNC\\vmware-host\\Shared Folders\\winda\\pciTxt\\pci.txt";
    ofstream out(outputPath);
    if (!out.is_open()) {
        cout << "Failed to open file for writing: " << outputPath << endl;
        system("pause");
        return 1;
    }

    HMODULE hWinIo = LoadLibraryA("C:\\WinIo\\Binaries\\WinIo32.dll");
    if (!hWinIo) {
        cout << "Failed to load WinIo32.dll" << endl;
        system("pause");
        return 1;
    }

    pInitializeWinIo InitializeWinIo = (pInitializeWinIo)GetProcAddress(hWinIo, "InitializeWinIo");
    pShutdownWinIo ShutdownWinIo = (pShutdownWinIo)GetProcAddress(hWinIo, "ShutdownWinIo");
    pGetPortVal GetPortVal = (pGetPortVal)GetProcAddress(hWinIo, "GetPortVal");
    pSetPortVal SetPortVal = (pSetPortVal)GetProcAddress(hWinIo, "SetPortVal");

    if (!InitializeWinIo || !ShutdownWinIo || !GetPortVal || !SetPortVal) {
        cout << "Failed to get function addresses from DLL!" << endl;
        FreeLibrary(hWinIo);
        system("pause");
        return 1;
    }

    if (!InitializeWinIo()) {
        cout << "Failed to initialize WinIo32" << endl;
        FreeLibrary(hWinIo);
        system("pause");
        return 1;
    }

    out << "PCI Devices List (VendorID / DeviceID):" << endl;

    for (short bus = 0; bus < 256; bus++) {
		count++;
        for (BYTE device = 0; device < 32; device++) {
            DWORD firstFuncData = pciConfigReadDword(SetPortVal, GetPortVal, bus, device, 0, 0);
			count++;
			if (firstFuncData == 0xFFFFFFFF){ 
				continue;
			}

            for (BYTE function = 0; function < 8; function++) {
                DWORD data = pciConfigReadDword(SetPortVal, GetPortVal, bus, device, function, 0);
				count++;
                WORD vendorID = (WORD)(data & 0xFFFF);
                WORD deviceID = (WORD)((data >> 16) & 0xFFFF);
                if (vendorID != 0xFFFF && vendorID != 0x0000) {
                    out << "Bus " << setw(3) << (int)bus
                        << ", Device " << setw(2) << (int)device
                        << ", Func " << (int)function
                        << " -> VendorID: 0x" << hex << setw(4) << setfill('0') << vendorID
                        << ", DeviceID: 0x" << hex << setw(4) << setfill('0') << deviceID
                        << dec << setfill(' ') << endl;
                }
            }

            cout << "Scanned Bus " << (int)bus << ", Device " << (int)device << endl;
        }
    }


   // FreeLibrary(hWinIo);
    out.close();
	cout << count << endl;
    cout << "Result saved to " << outputPath << endl;
    system("pause");
    return 0;
}