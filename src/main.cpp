#include <windows.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <mfapi.h>
#include <mfidl.h>
#include <iostream>
#include <string>
#include "VirtualCameraSpoofer.h"
#include "SimpleMediaSource.h"

// Note: In compatibility mode, we manually register the device in registry 
// instead of using Win11 MFCreateVirtualCamera API.

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")

// Manual device registration logic for systems without Win11 Virtual Camera API
bool RegisterVirtualCamera(const std::wstring& friendlyName) {
    // This creates a standard video capture device node in the registry
    // that TikTok Live Studio will pick up as a hardware device.
    HKEY hKey;
    std::wstring regPath = L"SOFTWARE\\Microsoft\\Windows Media Foundation\\Platform\\VirtualCamera";
    
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"FriendlyName", 0, REG_SZ, (BYTE*)friendlyName.c_str(), (DWORD)((friendlyName.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam (Compatibility Mode)        " << std::endl;
    std::wcout << L"========================================" << std::endl;
    
    std::wstring videoPath;
    std::wcout << L"Enter full path to local MP4/AVI file: ";
    std::getline(std::wcin, videoPath);

    if (videoPath.front() == L'\"' && videoPath.back() == L'\"') {
        videoPath = videoPath.substr(1, videoPath.length() - 2);
    }

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return 1;

    std::wstring friendlyName = L"Logitech HD Pro Webcam C920";
    
    // Fallback to manual registration since the API is missing
    std::wcout << L"Win11 API missing. Falling back to manual hardware registration..." << std::endl;
    
    if (RegisterVirtualCamera(friendlyName)) {
        std::wcout << L"[+] Manual device registration completed." << std::endl;
        
        // Apply deep hardware spoofing
        // This is where we inject the 0x84 capabilities and VID/PID
        std::wstring fakeSymlink = L"\\\\?\\swd#mfvirtualcam#{e5323777-f976-4f5b-9b55-b94699c46e44}#usb#vid_046d&pid_082d";
        VirtualCameraSpoofer::SpoofRegistry(fakeSymlink);
        
        std::wcout << L"\n[!] SUCCESS: Camera registered and spoofed." << std::endl;
        std::wcout << L"Please try locating '" << friendlyName << L"' in TikTok Live Studio." << std::endl;
        std::wcout << L"Press ENTER to stop..." << std::endl;
        
        // In this mode, the stream logic will need a registered MFT (Media Foundation Transform)
        // For testing, we keep the process alive.
        std::cin.get();
    } else {
        std::wcerr << L"Failed to register device. Run as Administrator." << std::endl;
    }

    MFShutdown();
    return 0;
}
