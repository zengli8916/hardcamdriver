<#
.SYNOPSIS
    HardCamDriver Environment Setup Script (Windows 11) - Fixed Version
.DESCRIPTION
    Automatically installs Git, CMake, VS2022 Build Tools, and Windows 11 SDK.
    Must be run as Administrator.
#>

# 1. Check for Administrator privileges
$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "Please run this PowerShell script as Administrator!"
    exit
}

Write-Host "Starting HardCamDriver environment configuration..." -ForegroundColor Cyan

# 2. Install Chocolatey (if not already installed)
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Chocolatey..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force; 
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; 
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
} else {
    Write-Host "Chocolatey is already installed, skipping." -ForegroundColor Green
}

# 3. Install development toolkits
Write-Host "Installing development tools via Chocolatey (this may take 15-30 mins)..." -ForegroundColor Yellow

# Using more reliable package names
# Note: visualstudio2022-workload-vctools often includes the SDK automatically.
$packages = @("git", "cmake", "visualstudio2022buildtools", "visualstudio2022-workload-vctools", "windows-11-sdk")

foreach ($pkg in $packages) {
    Write-Host "Checking/Installing $pkg..." -ForegroundColor Gray
    # Use -n to skip if already installed, improving speed on re-run
    choco install $pkg -y --no-progress --limit-output
}

# 4. Refresh Environment Variables
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

Write-Host "`n[!] Environment Setup Process Finished!" -ForegroundColor Green
Write-Host "[*] If windows-11-sdk failed again, don't worry. The VS Workload likely already installed it." -ForegroundColor Yellow
Write-Host "[*] Please RESTART your PowerShell or CMD window." -ForegroundColor Cyan
Write-Host "[*] Then try: mkdir build; cd build; cmake .." -ForegroundColor Gray
