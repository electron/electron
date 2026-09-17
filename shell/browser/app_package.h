// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_APP_PACKAGE_H_
#define ELECTRON_SHELL_BROWSER_APP_PACKAGE_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"

namespace electron {

// The app the browser process is going to run: the first of
// resources/app.asar, resources/app, resources/default_app.asar (or only
// app.asar under the OnlyLoadAppFromAsar fuse) with a readable package.json.
struct AppPackage {
  base::FilePath path;
  // package.json "main", or index.js.
  std::string main;
  // Whether |main| is loaded as an ES module.
  bool esm = false;
  // package.json "v8Flags"; applied just before |main| is loaded so that the
  // bundled startup scripts still match their code cache.
  std::string v8_flags;
};

// Finds the app and applies its package.json to the process: name, version,
// the Linux desktop name, and on Windows the Squirrel app user model id.
// nullopt if no candidate has a readable package.json.
std::optional<AppPackage> LoadAppPackage();

// The .desktop file name derived from an app name when package.json does not
// give one: lowercase ASCII words joined by '-', else the executable's name.
std::string DefaultDesktopName(const std::u16string& app_name);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_APP_PACKAGE_H_
