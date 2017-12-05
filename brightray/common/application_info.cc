#include "brightray/common/application_info.h"

namespace brightray {

namespace {

std::string overriden_application_name_;

}

void OverrideApplicationName(const std::string& name) {
    overriden_application_name_ = name;
}
std::string GetOverridenApplicationName() {
    return overriden_application_name_;
}

}  // namespace brightray
