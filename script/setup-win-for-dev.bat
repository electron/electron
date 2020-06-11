REM Parameters vs_buildtools.exe download link and wsdk version
@ECHO OFF

SET buildtools_link=https://download.visualstudio.microsoft.com/download/pr/d7691cc1-82e6-434f-8e9f-a612f85b4b76/c62179f8cbbb58d4af22c21e8d4e122165f21615f529c94fad5cc7e012f1ef08/vs_BuildTools.exe
SET wsdk10_link=https://go.microsoft.com/fwlink/p/?LinkId=845298
SET wsdk=10SDK.18362

REM Check for disk space
Rem        543210987654321
Set "Blank=               "
Set "GB100=   107374182400"

for /f "tokens=2" %%A in (
  'wmic LogicalDisk Get DeviceID^,FreeSpace ^| find /i "C:"'
) Do Set "FreeSpace=%Blank%%%A"
Set "FreeSpace=%FreeSpace:~-15%"

Echo FreeSpace="%FreeSpace%"
Echo    100 GB="%GB100%"

If "%FreeSpace%" gtr "%GB100%" (
  Echo yes enough free space
) else (
  Echo not enough free space - 100GB
  exit 5
)

REM Interpret arguments
:loop
IF NOT "%1"=="" (
    IF "%1"=="-buildtools_link" (
        SET buildtools_link=%2
        SHIFT
    )
    IF "%1"=="-wsdk" (
        SET wsdk=%2
        SHIFT
    )
    SHIFT
    GOTO :loop
)

@ECHO ON

if not exist "C:\TEMP\" mkdir C:\TEMP

REM Download vs_buildtools.exe to C:\TEMP\vs_buildtools.exe
powershell -command "& { iwr %buildtools_link% -OutFile C:\TEMP\vs_buildtools.exe }"

REM Install Visual Studio Toolchain
C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache ^
    --installPath "%ProgramFiles(x86)%/Microsoft Visual Studio/2019/Community" ^
    --add Microsoft.VisualStudio.Workload.VCTools ^
    --add Microsoft.VisualStudio.Component.VC.140 ^
    --add Microsoft.VisualStudio.Component.VC.ATLMFC ^
    --add Microsoft.VisualStudio.Component.VC.Tools.ARM64 ^
    --add Microsoft.VisualStudio.Component.VC.MFC.ARM64 ^
    --add Microsoft.VisualStudio.Component.Windows%wsdk% ^
    --includeRecommended

REM Install Windows SDK
powershell -command "& { iwr %wsdk10_link% -OutFile C:\TEMP\wsdk10.exe }"
C:\TEMP\wsdk10.exe /features /quiet

REM Install chocolatey to further install dependencies
set chocolateyUseWindowsCompression='true'
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" ^
    -NoProfile -InputFormat None -ExecutionPolicy Bypass ^
    -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

REM Install nodejs python git and yarn needed dependencies
choco install -y nodejs python2 git yarn windows-sdk-10-version-1903-windbg
call C:\ProgramData\chocolatey\bin\RefreshEnv.cmd
SET PATH=C:\Python27\;C:\Python27\Scripts;%PATH%

pip install pywin32
call C:\ProgramData\chocolatey\bin\RefreshEnv.cmd
pip2 install pywin32

REM Setup Depot Tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git C:\depot_tools
SET PATH=%PATH%;C:\depot_tools\