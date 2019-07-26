// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/platform_util.h"

#include <stdio.h>

#include "base/cancelable_callback.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/nix/xdg_util.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "url/gurl.h"

#define ELECTRON_TRASH "ELECTRON_TRASH"

namespace {

bool XDGUtil(const std::vector<std::string>& argv, const bool wait_for_exit) {
  base::LaunchOptions options;
  options.allow_new_privs = true;
  // xdg-open can fall back on mailcap which eventually might plumb through
  // to a command that needs a terminal.  Set the environment variable telling
  // it that we definitely don't have a terminal available and that it should
  // bring up a new terminal if necessary.  See "man mailcap".
  options.environment["MM_NOTTTY"] = "1";

  base::Process process = base::LaunchProcess(argv, options);
  if (!process.IsValid())
    return false;

  if (wait_for_exit) {
    int exit_code = -1;
    if (!process.WaitForExit(&exit_code))
      return false;
    return (exit_code == 0);
  }

  base::EnsureProcessGetsReaped(std::move(process));
  return true;
}

bool XDGOpen(const std::string& path, const bool wait_for_exit) {
  return XDGUtil({"xdg-open", path}, wait_for_exit);
}

bool XDGEmail(const std::string& email, const bool wait_for_exit) {
  return XDGUtil({"xdg-email", email}, wait_for_exit);
}

}  // namespace

namespace platform_util {

// TODO(estade): It would be nice to be able to select the file in the file
// manager, but that probably requires extending xdg-open. For now just
// show the folder.
void ShowItemInFolder(const base::FilePath& full_path) {
  base::FilePath dir = full_path.DirName();
  if (!base::DirectoryExists(dir))
    return;

  XDGOpen(dir.value(), false);
}

bool OpenItem(const base::FilePath& full_path) {
  return XDGOpen(full_path.value(), false);
}

void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  OpenExternalCallback callback) {
  // Don't wait for exit, since we don't want to wait for the browser/email
  // client window to close before returning
  if (url.SchemeIs("mailto"))
    std::move(callback).Run(XDGEmail(url.spec(), false) ? ""
                                                        : "Failed to open");
  else
    std::move(callback).Run(XDGOpen(url.spec(), false) ? "" : "Failed to open");
}

bool MoveItemToTrash(const base::FilePath& full_path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  // find the trash method
  std::string trash;
  if (!env->GetVar(ELECTRON_TRASH, &trash)) {
    // Determine desktop environment and set accordingly.
    const auto desktop_env(base::nix::GetDesktopEnvironment(env.get()));
    if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE4 ||
        desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE5) {
      trash = "kioclient5";
    } else if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE3) {
      trash = "kioclient";
    }
  }

  // build the invocation
  std::vector<std::string> argv;
  const auto& filename = full_path.value();
  if (trash == "kioclient5" || trash == "kioclient") {
    argv = {trash, "move", filename, "trash:/"};
  } else if (trash == "trash-cli") {
    argv = {"trash-put", filename};
  } else if (trash == "gvfs-trash") {
    argv = {"gvfs-trash", filename};  // deprecated, but still exists
  } else {
    argv = {"gio", "trash", filename};
  }

  return XDGUtil(argv, true);
}

void Beep() {
  // echo '\a' > /dev/console
  FILE* fp = fopen("/dev/console", "a");
  if (fp == nullptr) {
    fp = fopen("/dev/tty", "a");
  }
  if (fp != nullptr) {
    fprintf(fp, "\a");
    fclose(fp);
  }
}

bool GetDesktopName(std::string* setme) {
  bool found = false;

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string desktop_id = libgtkui::GetDesktopName(env.get());
  constexpr char const* libcc_default_id = "chromium-browser.desktop";
  if (!desktop_id.empty() && (desktop_id != libcc_default_id)) {
    *setme = desktop_id;
    found = true;
  }

  return found;
}

}  // namespace platform_util
