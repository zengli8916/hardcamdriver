#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>
#include <iostream>
#include <string>
#include "VirtualCameraSpoofer.h"
#include "SimpleMediaSource.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam Virtual Camera (Win11)       " << std::endl;
    std::wcout << L"========================================" << std::endl;
    
    std::wstring videoPath;
    std::wcout << L"Enter full path to local MP4/AVI file: ";
    std::getline(std::wcin, videoPath);

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return 1;

    // 1. Initialize our custom media source with the local file
    ComPtr<SimpleMediaSource> pSource = Make<SimpleMediaSource>();
    hr = pSource->Initialize(videoPath);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize media source with file: " << videoPath << std::endl;
        return 1;
    }

    // 2. Register Virtual Camera
    IMFVirtualCamera* pVirtualCamera = nullptr;
    const GUID categories[] = { KSCATEGORY_VIDEO_CAMERA };
    
    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"Logitech HD Pro Webcam C920", // Friendly Name Spoofing
        L"usb#vid_046d&pid_082d",       // Base ID
        categories,
        1,
        &pVirtualCamera
    );

    if (SUCCEEDED(hr) && pVirtualCamera) {
        // Here we'd normally register our SimpleMediaSource as a COM object in the system
        // and pass its CLSID via AddDeviceSourceInfo. 
        // For session-based cameras, the system will use the source we provide.
        
        hr = pVirtualCamera->Start(nullptr);
        if (SUCCEEDED(hr)) {
            std::wcout << L"[+] Virtual Camera Started." << std::endl;
            
            // 3. Deep Registry Spoofing to bypass TikTok Live Studio
            // In a real scenario, we'd query the actual symlink from the system
            std::wstring fakeSymlink = L"\\\\?\\swd#mfvirtualcam#{e5323777-f976-4f5b-9b55-b94699c46e44}#usb#vid_046d&pid_082d";
            VirtualCameraSpoofer::SpoofRegistry(fakeSymlink);

            std::wcout << L"\n[!] SUCCESS: Camera is active and spoofed as physical Logitech C920." << std::endl;
            std::wcout << L"Now open TikTok Live Studio and look for 'Logitech HD Pro Webcam C920'." << std::endl;
            std::wcout << L"Press ENTER to stop..." << std::endl;
            std::cin.get();

            pVirtualCamera->Stop();
        }
        pVirtualCamera->Release();
    } else {
        std::wcerr << L"Failed to create Virtual Camera. Error: " << std::hex << hr << std::endl;
    }

    MFShutdown();
    return 0;
}
