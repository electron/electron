// Copyright (c) 2026 Mitchell Cohen.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_GTK_GTK_SYMBOLIC_ICON_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_UI_GTK_GTK_SYMBOLIC_ICON_PROVIDER_H_

#include <string>

#include "shell/browser/ui/views/freedesktop_nav_button_provider.h"

namespace electron {

// SymbolicIconProvider backed by the GTK 3 API already loaded into the
// process by the LinuxUI toolkit backend.
class GtkSymbolicIconProvider : public SymbolicIconProvider {
 public:
  // Returns the process-lifetime instance, or null when the process has not
  // loaded GTK 3.
  static SymbolicIconProvider* GetInstance();

  GtkSymbolicIconProvider(const GtkSymbolicIconProvider&) = delete;
  GtkSymbolicIconProvider& operator=(const GtkSymbolicIconProvider&) = delete;

  // SymbolicIconProvider:
  SkBitmap LoadIcon(const std::string& icon_name,
                    int icon_size,
                    int scale,
                    SkColor color) override;
  std::string GetThemeName() override;

 private:
  GtkSymbolicIconProvider() = default;
  ~GtkSymbolicIconProvider() = default;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_GTK_GTK_SYMBOLIC_ICON_PROVIDER_H_
