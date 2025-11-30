#pragma once
#include"../Source.hpp"
enum TargetType { ANDROID, WINDOWS, UNKNOWN };

static const GUID SPP_UUID = { 0x00001101, 0x0000, 0x1000, {0x80,0x00,0x00,0x80,0x5F,0x9B,0x34,0xFB} };

struct BluetoothDevice {
    SOCKADDR_BTH address;
    std::wstring name;
    std::string addressStr;
    std::string deviceClass;
    DWORD classOfDevice;
    TargetType targetType;
};

std::string CoDMajorToString(DWORD classOfDevice);
std::string BtAddrToString(ULONGLONG btAddr);
TargetType DetectTargetType(const std::wstring& deviceName, DWORD classOfDevice, const std::string& deviceClass);
std::string TargetTypeToString(TargetType type);
void ListBluetoothDevices(std::vector<BluetoothDevice>& devices);
bool GetRfcommChannel(const SOCKADDR_BTH& deviceAddr, UCHAR& outChannel);
bool ConnectToDevice(const SOCKADDR_BTH& deviceAddr, TargetType target, SOCKET& outSocket);
bool ConnectToAndroid(const SOCKADDR_BTH& deviceAddr, SOCKET& outSocket);
void SendFile(const std::string& filePath, const BluetoothDevice& device);
void ReceiveFile(SOCKET clientSocket);
void RunServer();
