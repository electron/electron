#include "brightray/common/application_info.h"

namespace brightray {

namespace {

std::string g_overriden_application_name;

}

void OverrideApplicationName(const std::string& name) {
  g_overriden_application_name = name;
}
std::string GetOverridenApplicationName() {
  return g_overriden_application_name;
}

}  // namespace brightray
