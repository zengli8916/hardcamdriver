# HardCamDriver (Windows 11 Virtual Camera)

这是一个为了在 Windows 11 环境下突破直播软件（如 TikTok Live Studio）严格虚拟摄像头检测而设计的硬核防检测项目。

## 核心原理

TikTok Live Studio 和其他高级的反作弊系统会通过检测注册表路径、设备硬件 ID (VID/PID) 以及设备 `Capabilities` 属性来判断一个摄像头是真实的 USB 物理设备还是虚拟的（如 OBS Virtual Camera）。

本方案基于 **Windows 11 原生 Media Foundation 虚拟摄像头架构 (MFCreateVirtualCamera)** 结合 **注册表深度伪装技术 (Registry Spoofing)**。

### 技术突破点：
1. **PnP / 物理能力注入**: 将系统分配给虚拟摄像头的 `Capabilities` 修改为 `0x84`（表示其具有即插即用和真实物理属性）。
2. **硬件 ID (VID/PID) 欺骗**: 在 `DeviceClasses` 和 `Enum` 树中强制注入类似真实硬件（如 Logitech C920: `USB\VID_046D&PID_082D`）的标识符。
3. **用户态便利性**: 不需要复杂的 WDM/AVStream 内核驱动开发，也不需要昂贵的 EV 驱动签名。所有操作在管理员权限的用户态程序中完成。

## 编译与测试说明

该项目提供了一个完整的控制端和注册表伪装实现，您需要在 Windows 11 环境下使用 Visual Studio 或 CMake 进行编译。

### 环境要求
* Windows 11 操作系统 (必须，依赖 `MFCreateVirtualCamera` API)
* Visual Studio 2019/2022 (带 C++ 桌面开发组件)
* Windows 11 SDK

### 编译步骤
1. 打开 `x64 Native Tools Command Prompt for VS`
2. 进入代码目录：
   ```cmd
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```
3. 编译完成后，您会得到 `HardCamControl.exe`。

### 运行
**务必以管理员身份运行 `HardCamControl.exe`**，否则程序无法修改 HKLM 注册表中的设备信息，伪装将会失败。

## 后续视频流推流 (Media Source) 补充说明

本代码库实现了**控制端与防检测层**。为了让 TikTok Live Studio 能实际看到您指定的 MP4 文件画面，需要编写一个 `IMFMediaSource` COM 组件，并将其注册到系统。然后在 `main.cpp` 中通过 `pVirtualCamera->AddDeviceSourceInfo(Your_COM_CLSID)` 将其实例化。
您可以使用微软官方提供的 `SimpleCameraSample` 中的 `VirtualCameraMediaSource` 作为视频帧填充的基础模板。

## 免责声明
本代码仅供学习 Windows Media Foundation 架构及系统安全研究使用。
