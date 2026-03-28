#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "VirtualCameraSpoofer.h"

// VERSION 9.0 - UVC KERNEL REDIRECTION (FORCED usbvideo.sys BINDING)

bool ForceUVCBinding(const std::wstring& hardwareId, const std::wstring& friendlyName) {
    HKEY hKey;
    std::wstring deviceInstance = L"ROOT\\USB\\0000";
    std::wstring enumPath = L"SYSTEM\\CurrentControlSet\\Enum\\" + deviceInstance;
    
    // 1. Forge the Enum node as a USB device
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, enumPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        std::vector<wchar_t> multiSz;
        multiSz.insert(multiSz.end(), hardwareId.begin(), hardwareId.end());
        multiSz.push_back(L'\0'); multiSz.push_back(L'\0');
        
        RegSetValueExW(hKey, L"HardwareID", 0, REG_MULTI_SZ, (BYTE*)multiSz.data(), (DWORD)(multiSz.size() * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"DeviceDesc", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
        
        // FAKE: Point to the standard USB Video Class driver
        RegSetValueExW(hKey, L"Service", 0, REG_SZ, (BYTE*)L"usbvideo", (DWORD)((8 + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"Class", 0, REG_SZ, (BYTE*)L"Camera", (DWORD)((6 + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"ClassGUID", 0, REG_SZ, (BYTE*)L"{ca3e7ab9-b4d3-46e5-8251-5a4a2d44ad64}", (DWORD)((38 + 1) * sizeof(wchar_t)));
        
        DWORD caps = 0x84; 
        RegSetValueExW(hKey, L"Capabilities", 0, REG_DWORD, (BYTE*)&caps, sizeof(DWORD));
        DWORD configFlags = 0x0;
        RegSetValueExW(hKey, L"ConfigFlags", 0, REG_DWORD, (BYTE*)&configFlags, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }

    // 2. Forge the Device Interface in the UVC category
    std::wstring uvcGuid = L"{69965BE0-5081-11CF-B843-0020AF06AD54}";
    std::wstring symLink = L"##?#ROOT#USB#0000#" + uvcGuid;
    std::wstring interfacePath = L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\" + uvcGuid + L"\\" + symLink;
    
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, interfacePath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"DeviceInstance", 0, REG_SZ, (BYTE*)deviceInstance.c_str(), (DWORD)((deviceInstance.length() + 1) * sizeof(wchar_t)));
        
        HKEY hControlKey;
        if (RegCreateKeyExW(hKey, L"Control", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hControlKey, NULL) == ERROR_SUCCESS) {
            DWORD linked = 1; 
            RegSetValueExW(hControlKey, L"Linked", 0, REG_DWORD, (BYTE*)&linked, sizeof(DWORD));
            RegCloseKey(hControlKey);
        }

        HKEY hParamKey;
        if (RegCreateKeyExW(hKey, L"Device Parameters", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hParamKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hParamKey, L"FriendlyName", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hParamKey);
        }

        RegCloseKey(hKey);
        return true;
    }

    return false;
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam 9.0 (UVC Kernel Redirect)   " << std::endl;
    std::wcout << L"========================================" << std::endl;

    std::wstring hardwareId = L"USB\\VID_046D&PID_082D";
    std::wstring friendlyName = L"Logitech HD Pro Webcam C920";

    std::wcout << L"Forcing kernel binding to usbvideo.sys..." << std::endl;

    if (ForceUVCBinding(hardwareId, friendlyName)) {
        std::wcout << L"[+] Kernel state forged. System now believes a UVC device is linked." << std::endl;
        std::wcout << L"\n[!] Final Verification:" << std::endl;
        std::wcout << L"1. Open Device Manager." << std::endl;
        std::wcout << L"2. Action -> Scan for hardware changes." << std::endl;
        std::wcout << L"3. Check for 'Logitech HD Pro Webcam C920'." << std::endl;
        std::wcout << L"\nPress ENTER to exit..." << std::endl;
        std::cin.get();
    } else {
        std::wcerr << L"Failed. Admin privileges required." << std::endl;
    }

    return 0;
}
