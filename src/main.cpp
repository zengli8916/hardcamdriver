#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "VirtualCameraSpoofer.h"

// VERSION 5.0 - DIRECT REGISTRY INJECTION (NO SWDEVICE API)

bool CreateDeviceRegistry(const std::wstring& hardwareId, const std::wstring& friendlyName) {
    HKEY hKey;
    // We inject directly into the Root enumerator tree
    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\Root\\Camera\\0000";
    
    LSTATUS status = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS) {
        std::wcerr << L"Failed to create registry key. Error: " << status << L". Run as Administrator!" << std::endl;
        return false;
    }

    std::vector<wchar_t> multiSz;
    multiSz.insert(multiSz.end(), hardwareId.begin(), hardwareId.end());
    multiSz.push_back(L'\0');
    multiSz.push_back(L'\0');
    RegSetValueExW(hKey, L"HardwareID", 0, REG_MULTI_SZ, (BYTE*)multiSz.data(), (DWORD)(multiSz.size() * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"DeviceDesc", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));

    std::wstring classGuid = L"{69965BE0-5081-11CF-B843-0020AF06AD54}";
    RegSetValueExW(hKey, L"Class", 0, REG_SZ, (BYTE*)L"Camera", (DWORD)((6 + 1) * sizeof(wchar_t)));
    RegSetValueExW(hKey, L"ClassGUID", 0, REG_SZ, (BYTE*)classGuid.c_str(), (DWORD)((classGuid.length() + 1) * sizeof(wchar_t)));

    DWORD caps = 0x84;
    RegSetValueExW(hKey, L"Capabilities", 0, REG_DWORD, (BYTE*)&caps, sizeof(DWORD));

    RegCloseKey(hKey);
    return true;
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam (Direct Root Injection 5.0) " << std::endl;
    std::wcout << L"========================================" << std::endl;

    std::wstring hardwareId = L"USB\\VID_046D&PID_082D";
    std::wstring friendlyName = L"Logitech HD Pro Webcam C920";

    if (CreateDeviceRegistry(hardwareId, friendlyName)) {
        std::wcout << L"[+] Hardware registration injected successfully." << std::endl;
        VirtualCameraSpoofer::SpoofRegistry(L"Root\\Camera\\0000");
        std::wcout << L"\n[!] SUCCESS: Hardware C920 injected." << std::endl;
        std::wcout << L"Press ENTER to exit..." << std::endl;
        std::cin.get();
    } else {
        std::wcerr << L"Failed. Ensure you are using Administrator privileges." << std::endl;
    }

    return 0;
}
