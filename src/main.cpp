#include <windows.h>
#include <swdevice.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <iostream>
#include <string>
#include <vector>
#include "VirtualCameraSpoofer.h"

// Define function pointers for SwDevice API to avoid linker errors
typedef HRESULT (WINAPI *PFN_SwDeviceCreate)(
    PCWSTR pszEnumeratorName,
    PCWSTR pszParentDeviceInstance,
    const SW_DEVICE_CREATE_INFO *pCreateInfo,
    ULONG cPropertyCount,
    const DEVPROPERTY *pProperties,
    const SW_DEVICE_CREATE_CALLBACK_INFO *pCallbackInfo,
    PVOID pContext,
    PHSWDEVICE phSwDevice
);

typedef VOID (WINAPI *PFN_SwDeviceClose)(HSWDEVICE hSwDevice);

// Global to hold the device handle
HSWDEVICE g_hSwDevice = NULL;
HANDLE g_hEvent = NULL;

VOID WINAPI SwDeviceCallback(HSWDEVICE hSwDevice, HRESULT hr, PVOID pContext, PCWSTR pszDeviceInstanceId) {
    if (SUCCEEDED(hr)) {
        std::wcout << L"[+] Hardware Node Created: " << pszDeviceInstanceId << std::endl;
        SetEvent(g_hEvent);
    } else {
        std::wcerr << L"[-] Failed to create Hardware Node. Error: 0x" << std::hex << hr << std::endl;
    }
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam (PnP Simulation Mode)       " << std::endl;
    std::wcout << L"========================================" << std::endl;

    // 1. Load CfgMgr32.dll for SwDevice API
    HMODULE hCfgMgr = LoadLibraryW(L"cfgmgr32.dll");
    if (!hCfgMgr) {
        std::wcerr << L"Failed to load cfgmgr32.dll" << std::endl;
        return 1;
    }

    auto pfnSwDeviceCreate = (PFN_SwDeviceCreate)GetProcAddress(hCfgMgr, "SwDeviceCreate");
    auto pfnSwDeviceClose = (PFN_SwDeviceClose)GetProcAddress(hCfgMgr, "SwDeviceClose");

    if (!pfnSwDeviceCreate || !pfnSwDeviceClose) {
        std::wcerr << L"SwDevice APIs not found in this Windows version." << std::endl;
        return 1;
    }

    g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // 2. Prepare Device Identity (The Spoofing Core)
    SW_DEVICE_CREATE_INFO createInfo = {0};
    createInfo.cbSize = sizeof(createInfo);
    createInfo.pszInstanceId = L"HardCam_Logitech_C920";
    
    // This is what makes it look like a real USB camera to the system
    const wchar_t* hardwareIds = L"USB\\VID_046D&PID_082D\0USB\\VID_046D&PID_082D&REV_2901\0";
    createInfo.pszzHardwareIds = hardwareIds;
    createInfo.pszDeviceDescription = L"Logitech HD Pro Webcam C920";
    createInfo.CapabilityFlags = SWDeviceCapabilitiesRemovable | SWDeviceCapabilitiesSilentInstall;

    SW_DEVICE_CREATE_CALLBACK_INFO callbackInfo = {0};
    callbackInfo.cbSize = sizeof(callbackInfo);
    callbackInfo.pfnCallback = SwDeviceCallback;

    // 3. Set Device Class (Video Camera)
    DEVPROPERTY props[1];
    DEVPROPKEY keyClassGuid = { { 0x43675d81, 0x51ef, 0x4920, { 0xad, 0x22, 0x6e, 0x52, 0xa3, 0x5a, 0x4e, 0xbe } }, 1 }; // DEVPKEY_Device_ClassGuid
    props[0].Key = keyClassGuid;
    props[0].Type = DEVPROP_TYPE_GUID;
    props[0].BufferSize = sizeof(GUID);
    props[0].Buffer = (PVOID)&KSCATEGORY_VIDEO_CAMERA;

    std::wcout << L"Registering Hardware Simulation..." << std::endl;

    HRESULT hr = pfnSwDeviceCreate(
        L"HTRMgr", 
        NULL, 
        &createInfo, 
        1, 
        props, 
        &callbackInfo, 
        NULL, 
        &g_hSwDevice
    );

    if (SUCCEEDED(hr)) {
        WaitForSingleObject(g_hEvent, 10000);
        
        std::wcout << L"[+] Device successfully injected into PnP tree." << std::endl;
        
        // Final Spoof: Inject the 0x84 Capabilities into the newly created node
        std::wstring fakeInstance = L"SWD\\HTRMgr\\HardCam_Logitech_C920";
        VirtualCameraSpoofer::SpoofRegistry(fakeInstance);

        std::wcout << L"\n[!] SUCCESS: Check Device Manager -> Cameras." << std::endl;
        std::wcout << L"The system now sees a REAL Logitech C920 connected via USB." << std::endl;
        std::wcout << L"Press ENTER to disconnect and cleanup..." << std::endl;
        
        std::cin.get();
        pfnSwDeviceClose(g_hSwDevice);
    } else {
        std::wcerr << L"Failed to create SwDevice. Error: 0x" << std::hex << hr << std::endl;
    }

    CloseHandle(g_hEvent);
    return 0;
}
