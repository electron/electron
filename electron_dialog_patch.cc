#include <string>
#include <vector>
#include <algorithm>

namespace electron {

std::vector<std::string> FixCustomPackageExtensions(const std::vector<std::string>& filters, bool is_macos_package_type) {
  std::vector<std::string> adjusted_filters = filters;
  if (is_macos_package_type) {
    for (auto& filter : adjusted_filters) {
      if (filter == "*") {
        filter = "*";
      }
    }
  }
  return adjusted_filters;
}

}  // namespace electron
