#include <windows.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
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

// Define the function pointer type for MFCreateVirtualCamera
typedef HRESULT (STDAPICALLTYPE *PFN_MFCreateVirtualCamera)(
    MFVirtualCameraType type,
    MFVirtualCameraLifetime lifetime,
    MFVirtualCameraAccess access,
    LPCWSTR friendlyName,
    LPCWSTR sourceId,
    const GUID* categories,
    ULONG categoryCount,
    IMFVirtualCamera** ppVirtualCamera
);

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam Virtual Camera (Win11)       " << std::endl;
    std::wcout << L"========================================" << std::endl;
    
    std::wstring videoPath;
    std::wcout << L"Enter full path to local MP4/AVI file: ";
    std::getline(std::wcin, videoPath);

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return 1;

    // Load mfplat.dll dynamically to get MFCreateVirtualCamera
    HMODULE hMfPlat = GetModuleHandleW(L"mfplat.dll");
    if (!hMfPlat) hMfPlat = LoadLibraryW(L"mfplat.dll");
    
    if (!hMfPlat) {
        std::wcerr << L"Failed to load mfplat.dll" << std::endl;
        return 1;
    }

    auto pfnMFCreateVirtualCamera = (PFN_MFCreateVirtualCamera)GetProcAddress(hMfPlat, "MFCreateVirtualCamera");
    if (!pfnMFCreateVirtualCamera) {
        std::wcerr << L"Critical Error: MFCreateVirtualCamera API NOT FOUND." << std::endl;
        std::wcerr << L"This usually means you are NOT running on Windows 11." << std::endl;
        return 1;
    }

    ComPtr<SimpleMediaSource> pSource = Make<SimpleMediaSource>();
    hr = pSource->Initialize(videoPath);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to initialize media source." << std::endl;
        return 1;
    }

    IMFVirtualCamera* pVirtualCamera = nullptr;
    const GUID categories[] = { KSCATEGORY_VIDEO_CAMERA };
    
    // Call the function via pointer
    hr = pfnMFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"Logitech HD Pro Webcam C920",
        L"usb#vid_046d&pid_082d",
        categories,
        1,
        &pVirtualCamera
    );

    if (SUCCEEDED(hr) && pVirtualCamera) {
        hr = pVirtualCamera->Start(nullptr);
        if (SUCCEEDED(hr)) {
            std::wcout << L"[+] Virtual Camera Started." << std::endl;
            
            // Note: In real scenarios, query the actual symlink. 
            // This is a placeholder for the spoofing logic.
            std::wstring fakeSymlink = L"\\\\?\\swd#mfvirtualcam#{e5323777-f976-4f5b-9b55-b94699c46e44}#usb#vid_046d&pid_082d";
            VirtualCameraSpoofer::SpoofRegistry(fakeSymlink);

            std::wcout << L"\n[!] SUCCESS: Camera is active." << std::endl;
            std::wcout << L"Press ENTER to stop..." << std::endl;
            std::cin.get();

            pVirtualCamera->Stop();
        }
        pVirtualCamera->Release();
    } else {
        std::wcerr << L"Failed to create Virtual Camera. Error: 0x" << std::hex << hr << std::endl;
    }

    MFShutdown();
    return 0;
}
