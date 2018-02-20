#include "brightray/common/application_info.h"

namespace brightray {

namespace {

std::string g_overriden_application_name;
std::string g_overridden_application_version;

}

void OverrideApplicationName(const std::string& name) {
  g_overriden_application_name = name;
}
std::string GetOverridenApplicationName() {
  return g_overriden_application_name;
}

void OverrideApplicationVersion(const std::string& version) {
  g_overridden_application_version = version;
}
std::string GetOverriddenApplicationVersion() {
  return g_overridden_application_version;
}

}  // namespace brightray
