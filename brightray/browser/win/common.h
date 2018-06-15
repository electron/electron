#pragma once
#include <Windows.h>
#include <appmodel.h>

namespace brightray {

namespace {

bool hasCheckedIsRunningInDesktopBridge = false;
bool isRunningInDesktopBridge = false;

bool IsRunningInDesktopBridge() {
  if (hasCheckedIsRunningInDesktopBridge) {
    return isRunningInDesktopBridge;
  }

  UINT32 length;
  wchar_t packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH + 1];
  LONG result = GetPackageFamilyName(GetCurrentProcess(), &length, packageFamilyName);
  isRunningInDesktopBridge = result == ERROR_SUCCESS;
  hasCheckedIsRunningInDesktopBridge = true;

  return isRunningInDesktopBridge;
}

}

} // namespace brightray
