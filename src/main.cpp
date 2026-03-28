#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>
#include <iostream>
#include "VirtualCameraSpoofer.h"

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

int main() {
    std::wcout << L"Initializing HardCam Virtual Camera Control..." << std::endl;
    
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to start Media Foundation." << std::endl;
        return 1;
    }

    // Attempt to create a virtual camera using Windows 11 APIs
    IMFVirtualCamera* pVirtualCamera = nullptr;
    
    std::wcout << L"Creating Virtual Camera Instance..." << std::endl;
    
    // Using KSCATEGORY_VIDEO_CAMERA to be picked up by most software
    const GUID categories[] = { KSCATEGORY_VIDEO_CAMERA };
    
    // Note: To make this push actual video frames from a file, a custom Media Source COM object 
    // must be registered in the system, and its CLSID linked here via pVirtualCamera->AddDeviceSourceInfo().
    // For this implementation, we focus on the camera registration and the registry spoofing logic.
    hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"Logitech HD Pro Webcam C920", // Friendly Name Spoofing for TikTok Live Studio
        L"usb#vid_046d&pid_082d",       // Fake source ID/Moniker base
        categories,
        1,
        &pVirtualCamera
    );

    if (SUCCEEDED(hr) && pVirtualCamera) {
        std::wcout << L"Virtual Camera Object created. Starting..." << std::endl;
        hr = pVirtualCamera->Start(nullptr);
        if (SUCCEEDED(hr)) {
            std::wcout << L"Camera started in the system. Applying deep registry spoofing..." << std::endl;

            // In a complete implementation, we'd query the actual allocated symlink
            // Here we assume a generated symlink path for demonstration
            std::wstring fakeSymlink = L"\\\\?\\swd#mfvirtualcam#{e5323777-f976-4f5b-9b55-b94699c46e44}#usb#vid_046d&pid_082d";
            
            // Bypass TikTok Live Studio detection by modifying the hardware characteristics in registry
            VirtualCameraSpoofer::SpoofRegistry(fakeSymlink);

            std::wcout << L"\n[!] Camera is active and spoofed." << std::endl;
            std::wcout << L"Press Enter to stop and remove the virtual camera..." << std::endl;
            std::cin.get();

            pVirtualCamera->Stop();
            std::wcout << L"Camera stopped." << std::endl;
        } else {
            std::wcerr << L"Failed to start camera. Error: " << std::hex << hr << std::endl;
        }
        pVirtualCamera->Release();
    } else {
        std::wcerr << L"Failed to create Virtual Camera. Ensure you are on Windows 11. Error: " << std::hex << hr << std::endl;
    }

    MFShutdown();
    return 0;
}
