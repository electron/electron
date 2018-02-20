#ifndef BRIGHTRAY_COMMON_APPLICATION_INFO_H_
#define BRIGHTRAY_COMMON_APPLICATION_INFO_H_

#if defined(OS_WIN)
#include "brightray/browser/win/scoped_hstring.h"
#endif

#include <string>

namespace brightray {

void OverrideApplicationName(const std::string& name);
std::string GetOverriddenApplicationName();

void OverrideApplicationVersion(const std::string& version);
std::string GetOverriddenApplicationVersion();

std::string GetApplicationName();
std::string GetApplicationVersion();

#if defined(OS_WIN)
PCWSTR GetRawAppUserModelID();
bool GetAppUserModelID(ScopedHString* app_id);
void SetAppUserModelID(const base::string16& name);
#endif

}  // namespace brightray

#endif  // BRIGHTRAY_COMMON_APPLICATION_INFO_H_
