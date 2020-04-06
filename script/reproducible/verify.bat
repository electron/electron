@ECHO OFF
set CHECK_VERSION=%1
SHIFT

set buildir=C:\projects\

REM Interpret arguments
:loop
IF NOT "%1"=="" (
    IF "%1"=="-buildir" (
        SET buildir=%2
        SHIFT
    )
    SHIFT
    GOTO :loop
)

@ECHO ON

set ELECTRON_OUT_DIR=Default
set ELECTRON_ENABLE_STACK_DUMPING=1

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

REM Install missing dependencies
powershell -command "& { iwr https://raw.githubusercontent.com/electron/electron/%CHECK_VERSION%/script/setup-win-for-dev.bat -OutFile C:\install.bat }"
cmd /c C:\install.bat


set GN_CONFIG=release

echo "Building $env:GN_CONFIG build"
SET PATH=%PATH%;C:\depot_tools\
SET PATH=%PATH%;C:\Python27\
git config --global core.longpaths true
if not exist %buildir% mkdir %buildir%
cd %buildir%
set CHROMIUM_BUILDTOOLS_PATH=%buildir%\src\buildtools
set SCCACHE_PATH=%buildir%\src\electron\external_binaries\sccache.exe
REM set GYP_MSVS_OVERRIDE_PATH=C:\BuildTools\
set GYP_MSVS_VERSION=2019
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
REM set GCLIENT_EXTRA_ARGS="%GCLIENT_EXTRA_ARGS% --custom-var=\"checkout_boto=True\" --custom-var=\"checkout_requests=True\""
cmd /C gclient config --name "src\electron" --unmanaged %GCLIENT_EXTRA_ARGS% "https://github.com/electron/electron"
cmd /C gclient sync -f --with_branch_heads --with_tags
cd src
cd electron
git fetch
git checkout %CHECK_VERSION%
cmd /c gclient sync -f
cd ..
cd ..
set BUILD_CONFIG_PATH="//electron/build/args/release.gn"
REM build out/Default
cd src
REM XXX: leave extra space after GN_EXTRA_ARG
set GN_EXTRA_ARG= 
cmd /C gn gen out/Default --args="import(\"//electron/build/args/release.gn\") %GN_EXTRA_ARG% cc_wrapper=\"%SCCACHE_PATH%\""
ninja -C out/Default electron:electron_app
cmd /C gn gen out/ffmpeg "--args=import(\"//electron/build/args/ffmpeg.gn\") %GN_EXTRA_ARGS%"
ninja -C out/ffmpeg electron:electron_ffmpeg_zip
ninja -C out/Default electron:electron_dist_zip
ninja -C out/Default electron:electron_mksnapshot_zip
ninja -C out/Default electron:hunspell_dictionaries_zip
ninja -C out/Default electron:electron_chromedriver_zip
ninja -C out/Default third_party/electron_node:headers
cmd /C %SCCACHE_PATH% --show-stats
cmd /C %SCCACHE_PATH% --stop-server

cd out
REM loop needed since sometimes the output directory is locked before completing the compilation
:renloop
timeout 1
ren Default Default2 || goto :renloop
ren ffmpeg ffmpeg2
cd ..

cmd /C gn gen out/Default --args="import(\"//electron/build/args/release.gn\") %GN_EXTRA_ARG% cc_wrapper=\"%SCCACHE_PATH%\""
ninja -C out/Default electron:electron_app
cmd /C gn gen out/ffmpeg "--args=import(\"//electron/build/args/ffmpeg.gn\") %GN_EXTRA_ARGS%"
ninja -C out/ffmpeg electron:electron_ffmpeg_zip
ninja -C out/Default electron:electron_dist_zip
ninja -C out/Default electron:electron_mksnapshot_zip
ninja -C out/Default electron:hunspell_dictionaries_zip
ninja -C out/Default electron:electron_chromedriver_zip
ninja -C out/Default third_party/electron_node:headers
cmd /C %SCCACHE_PATH% --show-stats

REM Check for local determinism, there is no reason in checking remote determinism if local is not satisfied
pip install python-stripzip
for %%f in (out/Default/*.zip) do (
  stripzip out/Default/%%f
)
for %%f in (out/Default2/*.zip) do (
  stripzip out/Default/%%f
)
python tools/determinism/compare_build_artifacts.py -f out/Default -s out/Default2

REM Check for remote determinism comparing output zip hashes
powershell -command "& { iwr https://github.com/electron/electron/releases/download/%CHECK_VERSION%/SHASUMS256.txt -OutFile C:\TEMP\SHASUMS256.txt }"

ECHO Expected SHA256SUM
type C:\TEMP\SHASUMS256.txt

ECHO Calculated SHA256SUM
for %%f in (out/Default/*.zip) do (
  certutil -hashfile out/Default/%%f SHA256
)
for %%f in (out/Default2/*.zip) do (
  certutil -hashfile out/Default/%%f SHA256
)
