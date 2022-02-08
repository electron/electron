REM Parameters vs_buildtools.exe download link and wsdk version
@ECHO OFF

SET wsdk10_link=https://go.microsoft.com/fwlink/?linkid=2120843
SET wsdk=10SDK.19041

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

REM Install chocolatey to further install dependencies
set chocolateyUseWindowsCompression='true'
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" ^
    -NoProfile -InputFormat None -ExecutionPolicy Bypass ^
    -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"

REM Install Visual Studio Toolchain
choco install visualstudio2019buildtools --package-parameters "--quiet --wait --norestart --nocache  --installPath ""%ProgramFiles(x86)%/Microsoft Visual Studio/2019/Community"" --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.ATLMFC --add Microsoft.VisualStudio.Component.VC.Tools.ARM64 --add Microsoft.VisualStudio.Component.VC.MFC.ARM64 --add Microsoft.VisualStudio.Component.Windows%wsdk% --includeRecommended"

REM Install Windows SDK
powershell -command "& { iwr %wsdk10_link% -OutFile C:\TEMP\wsdk10.exe }"
C:\TEMP\wsdk10.exe /features /quiet

REM Install nodejs python git and yarn needed dependencies
choco install -y nodejs python2 git yarn
choco install windows-sdk-10-version-2004-windbg
call C:\ProgramData\chocolatey\bin\RefreshEnv.cmd
SET PATH=C:\Python27\;C:\Python27\Scripts;%PATH%

REM Install pywin32 and pypiwin32
python -m pip install -U pypiwin32
@REM pip install pypiwin32
python Scripts\pywin32_postinstall.py -install
call C:\ProgramData\chocolatey\bin\RefreshEnv.cmd
python -m pip2 install -U pypiwin32
@REM pip2 install pypiwin32
python Scripts\pywin32_postinstall.py -install

REM Setup Depot Tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git C:\depot_tools
SET PATH=%PATH%;C:\depot_tools\