#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "VirtualCameraSpoofer.h"

// --- Custom Names to avoid any collision with existing/partial SDK headers ---

typedef enum _HC_SW_DEVICE_CAPABILITIES {
    HC_SWDeviceCapabilitiesNone = 0x00000000,
    HC_SWDeviceCapabilitiesRemovable = 0x00000001,
    HC_SWDeviceCapabilitiesSilentInstall = 0x00000002,
    HC_SWDeviceCapabilitiesNoDisplayInUI = 0x00000004,
    HC_SWDeviceCapabilitiesDriverRequired = 0x00000008
} HC_SW_DEVICE_CAPABILITIES;

typedef struct _HC_SW_DEVICE_CREATE_INFO {
    ULONG cbSize;
    PCWSTR pszInstanceId;
    PCWSTR pszDisplayName;
    PCWSTR pszzHardwareIds;
    PCWSTR pszzCompatibleIds;
    const GUID *pContainerId;
    ULONG CapabilityFlags;
    PCWSTR pszDeviceDescription;
    PCWSTR pszDeviceLocation;
    const GUID *pSecurityDescriptor;
} HC_SW_DEVICE_CREATE_INFO;

typedef VOID (WINAPI *HC_SW_DEVICE_CREATE_CALLBACK)(void* hSwDevice, HRESULT hr, PVOID pContext, PCWSTR pszDeviceInstanceId);

typedef struct _HC_SW_DEVICE_CREATE_CALLBACK_INFO {
    ULONG cbSize;
    HC_SW_DEVICE_CREATE_CALLBACK pfnCallback;
    PVOID pContext;
} HC_SW_DEVICE_CREATE_CALLBACK_INFO;

typedef struct _HC_DEVPROPERTY {
    DEVPROPKEY Key;
    DEVPROPTYPE Type;
    ULONG BufferSize;
    PVOID Buffer;
} HC_DEVPROPERTY;

// API Function Pointers
typedef HRESULT (WINAPI *PFN_SwDeviceCreate)(
    PCWSTR pszEnumeratorName,
    PCWSTR pszParentDeviceInstance,
    const HC_SW_DEVICE_CREATE_INFO *pCreateInfo,
    ULONG cPropertyCount,
    const HC_DEVPROPERTY *pProperties,
    const HC_SW_DEVICE_CREATE_CALLBACK_INFO *pCallbackInfo,
    PVOID pContext,
    void** phSwDevice
);

typedef VOID (WINAPI *PFN_SwDeviceClose)(void* hSwDevice);

// --- Global variables ---
void* g_hSwDevice = NULL;
HANDLE g_hEvent = NULL;

VOID WINAPI SwDeviceCallback(void* hSwDevice, HRESULT hr, PVOID pContext, PCWSTR pszDeviceInstanceId) {
    if (SUCCEEDED(hr)) {
        std::wcout << L"[+] Hardware Node Created: " << pszDeviceInstanceId << std::endl;
        SetEvent(g_hEvent);
    } else {
        std::wcerr << L"[-] Failed to create Hardware Node. Error: 0x" << std::hex << hr << std::endl;
    }
}

int main() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"   HardCam (Robust PnP Mode 3.0)       " << std::endl;
    std::wcout << L"========================================" << std::endl;

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

    HC_SW_DEVICE_CREATE_INFO createInfo = {0};
    createInfo.cbSize = sizeof(createInfo);
    createInfo.pszInstanceId = L"HardCam_Logitech_C920";
    createInfo.pszzHardwareIds = L"USB\\VID_046D&PID_082D\0USB\\VID_046D&PID_082D&REV_2901\0";
    createInfo.pszDeviceDescription = L"Logitech HD Pro Webcam C920";
    createInfo.CapabilityFlags = HC_SWDeviceCapabilitiesRemovable | HC_SWDeviceCapabilitiesSilentInstall;

    HC_SW_DEVICE_CREATE_CALLBACK_INFO callbackInfo = {0};
    callbackInfo.cbSize = sizeof(callbackInfo);
    callbackInfo.pfnCallback = (HC_SW_DEVICE_CREATE_CALLBACK)SwDeviceCallback;

    HC_DEVPROPERTY props[1];
    // DEVPKEY_Device_ClassGuid
    DEVPROPKEY keyClassGuid = { { 0x43675d81, 0x51ef, 0x4920, { 0xad, 0x22, 0x6e, 0x52, 0xa3, 0x5a, 0x4e, 0xbe } }, 1 }; 
    // KSCATEGORY_VIDEO_CAMERA: {69965BE0-5081-11CF-B843-0020AF06AD54}
    GUID guidVideoCamera = { 0x69965BE0, 0x5081, 0x11CF, { 0xB8, 0x43, 0x00, 0x20, 0xAF, 0x06, 0xAD, 0x54 } };

    props[0].Key = keyClassGuid;
    props[0].Type = DEVPROP_TYPE_GUID;
    props[0].BufferSize = sizeof(GUID);
    props[0].Buffer = (PVOID)&guidVideoCamera;

    std::wcout << L"Registering Hardware Simulation..." << std::endl;

    HRESULT hr = pfnSwDeviceCreate(L"HTRMgr", NULL, &createInfo, 1, props, &callbackInfo, NULL, &g_hSwDevice);

    if (SUCCEEDED(hr)) {
        WaitForSingleObject(g_hEvent, 10000);
        
        std::wstring fakeInstance = L"SWD\\HTRMgr\\HardCam_Logitech_C920";
        VirtualCameraSpoofer::SpoofRegistry(fakeInstance);

        std::wcout << L"\n[!] SUCCESS: Hardware C920 is now VISIBLE." << std::endl;
        std::wcout << L"Press ENTER to exit..." << std::endl;
        
        std::cin.get();
        pfnSwDeviceClose(g_hSwDevice);
    } else {
        std::wcerr << L"Failed. Error: 0x" << std::hex << hr << std::endl;
    }

    CloseHandle(g_hEvent);
    return 0;
}
