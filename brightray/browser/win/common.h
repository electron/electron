#pragma once
#include <Windows.h>
#include <appmodel.h>

namespace brightray {

namespace {

bool isRunningInDesktopBridge() {
  UINT32 length = 0;
  LONG rc = GetPackageFamilyName(GetCurrentProcess(), &length, NULL);

  if (rc != ERROR_INSUFFICIENT_BUFFER) {
    if (rc == APPMODEL_ERROR_NO_PACKAGE) {
      // Process has no package identity, the expected
      // win32 case
      return false;
    } else {
      // A failure is also a *very* strong indicator
      // that we're not in the desktop bridge
      return false;
    }
  }

  // We have an identity!
  return true;
}

}

} // namespace brightray
