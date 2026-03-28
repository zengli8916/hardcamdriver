#include "VirtualCameraSpoofer.h"
#include <iostream>
#include <vector>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

// Helper to write string to registry
bool WriteRegString(HKEY hKey, const std::wstring& valueName, const std::wstring& data) {
    LSTATUS status = RegSetValueExW(
        hKey,
        valueName.c_str(),
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(data.c_str()),
        static_cast<DWORD>((data.length() + 1) * sizeof(wchar_t))
    );
    return status == ERROR_SUCCESS;
}

// Helper to write DWORD to registry
bool WriteRegDword(HKEY hKey, const std::wstring& valueName, DWORD data) {
    LSTATUS status = RegSetValueExW(
        hKey,
        valueName.c_str(),
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&data),
        sizeof(DWORD)
    );
    return status == ERROR_SUCCESS;
}

std::wstring VirtualCameraSpoofer::ExtractDeviceInstanceFromSymlink(const std::wstring& symlink) {
    // Example Symlink: \\?\swd#mfvirtualcam#{e5323777-f976-4f5b-9b55-b94699c46e44}#{GUID}
    // We parse it to find the base device instance string.
    std::wstring instance = symlink;
    size_t prefix_pos = instance.find(L"\\\\?\\");
    if (prefix_pos == 0) {
        instance = instance.substr(4); // Remove prefix
    }
    
    // Replace '#' with '\' to match registry path
    for (auto& c : instance) {
        if (c == L'#') {
            c = L'\\';
        }
    }
    
    // Usually, the instance ends at the second backslash for the main device node
    return instance;
}

bool VirtualCameraSpoofer::SpoofRegistry(const std::wstring& symbolicLink) {
    std::wcout << L"[Spoofer] Target Symlink: " << symbolicLink << std::endl;
    std::wstring deviceInstance = ExtractDeviceInstanceFromSymlink(symbolicLink);
    
    // Target registry path: HKLM\SYSTEM\CurrentControlSet\Enum\[DeviceInstance]
    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\" + deviceInstance;
    
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey);
    
    if (status != ERROR_SUCCESS) {
        std::wcerr << L"[-] Failed to open device registry key. Ensure the program is run as Administrator. Error: " << status << std::endl;
        return false;
    }
    
    // 1. Inject physical Capabilities (0x84 = PnP + Removable/Physical)
    // This is a key trick mentioned in the PDF documentation to avoid software detection.
    DWORD fakeCapabilities = 0x84; 
    WriteRegDword(hKey, L"Capabilities", fakeCapabilities);
    std::wcout << L"[+] Injected fake physical Capabilities (0x84)." << std::endl;
    
    RegCloseKey(hKey);

    // 2. We also need to manipulate the DeviceClasses to ensure it's loaded as a hardware cam
    // This is part of the KSCATEGORY_VIDEO_CAMERA spoofing.
    std::wstring classPath = L"SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{E5323777-F976-4f5b-9B55-B94699C46E44}\\##?#" + symbolicLink.substr(4);
    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, classPath.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey);
    
    if (status == ERROR_SUCCESS) {
        WriteRegString(hKey, L"DeviceInstance", L"USB\\VID_046D&PID_082D\\12345678"); // Fake Logitech C920
        std::wcout << L"[+] Injected DeviceInstance spoof to DeviceClasses." << std::endl;
        RegCloseKey(hKey);
    }

    return true;
}

bool VirtualCameraSpoofer::SetHardwareID(const std::wstring& devicePath, const std::wstring& fakeVidPid) {
    // Open the property store for the virtual camera in registry
    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Enum\\" + devicePath;
    HKEY hKey = nullptr;
    LSTATUS status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey);
    
    if (status != ERROR_SUCCESS) return false;
    
    // Standard hardware IDs list (MULTI_SZ)
    // We use REG_MULTI_SZ format: string1 \0 string2 \0 \0
    std::vector<wchar_t> multiSz;
    multiSz.insert(multiSz.end(), fakeVidPid.begin(), fakeVidPid.end());
    multiSz.push_back(L'\0');
    multiSz.push_back(L'\0'); // Double null termination

    status = RegSetValueExW(
        hKey,
        L"HardwareID",
        0,
        REG_MULTI_SZ,
        reinterpret_cast<const BYTE*>(multiSz.data()),
        static_cast<DWORD>(multiSz.size() * sizeof(wchar_t))
    );
    
    if (status == ERROR_SUCCESS) {
         std::wcout << L"[+] HardwareID spoofed to: " << fakeVidPid << std::endl;
    }
    
    RegCloseKey(hKey);
    return status == ERROR_SUCCESS;
}
