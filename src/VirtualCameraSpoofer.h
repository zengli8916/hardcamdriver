#pragma once
#include <windows.h>
#include <string>

class VirtualCameraSpoofer {
public:
    /**
     * @brief Modifies the registry properties of the virtual camera to simulate physical hardware.
     * @param symbolicLink The device symlink string returned by Media Foundation.
     * @return true if successful, false otherwise.
     */
    static bool SpoofRegistry(const std::wstring& symbolicLink);

    /**
     * @brief Sets fake physical hardware IDs (e.g., VID/PID) to bypass TikTok Live Studio's checks.
     * @param devicePath The internal path derived from the symbolic link.
     * @param fakeVidPid The target VID/PID string (e.g., "USB\\VID_046D&PID_082D").
     * @return true if successful, false otherwise.
     */
    static bool SetHardwareID(const std::wstring& devicePath, const std::wstring& fakeVidPid);

private:
    static std::wstring ExtractDeviceInstanceFromSymlink(const std::wstring& symlink);
};
