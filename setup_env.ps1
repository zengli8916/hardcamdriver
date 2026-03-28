<#
.SYNOPSIS
    HardCamDriver 开发环境一键配置脚本 (Windows 11)
.DESCRIPTION
    该脚本将自动安装 Git, CMake, Visual Studio 2022 Build Tools 和 Windows 11 SDK。
    必须以管理员权限运行。
#>

# 1. 检查管理员权限
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "请以管理员身份运行此 PowerShell 脚本！"
    exit
}

Write-Host "开始配置 HardCamDriver 开发环境..." -ForegroundColor Cyan

# 2. 安装 Chocolatey (如果不存在)
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "正在安装 Chocolatey..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force; 
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; 
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
} else {
    Write-Host "Chocolatey 已安装，跳过。" -ForegroundColor Green
}

# 3. 安装开发工具包
Write-Host "正在通过 Chocolatey 安装开发工具 (这可能需要较长时间)..." -ForegroundColor Yellow

# Git, CMake, VS2022 Build Tools, Windows 11 SDK
$packages = @("git", "cmake", "visualstudio2022buildtools", "visualstudio2022-workload-vctools", "windows-11-sdk-22000")

foreach ($pkg in $packages) {
    Write-Host "正在安装 $pkg..." -ForegroundColor Gray
    choco install $pkg -y --no-progress
}

# 4. 刷新环境变量
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "`n[!] 环境配置完成！" -ForegroundColor Green
Write-Host "[*] 请重新启动 PowerShell 或 CMD 以使所有路径生效。" -ForegroundColor Cyan
Write-Host "[*] 然后您可以运行 'cmake --version' 和 'git --version' 进行验证。" -ForegroundColor Gray
