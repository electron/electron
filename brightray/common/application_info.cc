#include "brightray/common/application_info.h"

namespace brightray {

namespace {

std::string g_overridden_application_name;
std::string g_overridden_application_version;

}  // namespace

// name
void OverrideApplicationName(const std::string& name) {
  g_overridden_application_name = name;
}
std::string GetOverriddenApplicationName() {
  return g_overridden_application_name;
}

// version
void OverrideApplicationVersion(const std::string& version) {
  g_overridden_application_version = version;
}
std::string GetOverriddenApplicationVersion() {
  return g_overridden_application_version;
}

}  // namespace brightray
