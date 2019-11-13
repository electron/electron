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

// Descriptions pulled from https://linux.die.net/man/1/xdg-open
std::string GetErrorDescription(int error_code) {
  switch (error_code) {
    case 1:
      return "Error in command line syntax";
    case 2:
      return "The item does not exist";
    case 3:
      return "A required tool could not be found";
    case 4:
      return "The action failed";
    default:
      return "";
  }
}

bool XDGUtil(const std::vector<std::string>& argv,
             const bool wait_for_exit,
             platform_util::OpenCallback callback) {
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
    bool success = process.WaitForExit(&exit_code);
    if (!callback.is_null())
      std::move(callback).Run(GetErrorDescription(exit_code));
    return success ? (exit_code == 0) : false;
  }

  base::EnsureProcessGetsReaped(std::move(process));
  return true;
}

bool XDGOpen(const std::string& path,
             const bool wait_for_exit,
             platform_util::OpenCallback callback) {
  return XDGUtil({"xdg-open", path}, wait_for_exit, std::move(callback));
}

bool XDGEmail(const std::string& email, const bool wait_for_exit) {
  return XDGUtil({"xdg-email", email}, wait_for_exit,
                 platform_util::OpenCallback());
}

}  // namespace

namespace platform_util {

void ShowItemInFolder(const base::FilePath& full_path) {
  base::FilePath dir = full_path.DirName();
  if (!base::DirectoryExists(dir))
    return;

  XDGOpen(dir.value(), false, platform_util::OpenCallback());
}

void OpenPath(const base::FilePath& full_path, OpenCallback callback) {
  // This is async, so we don't care about the return value.
  XDGOpen(full_path.value(), true, std::move(callback));
}

void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  OpenCallback callback) {
  // Don't wait for exit, since we don't want to wait for the browser/email
  // client window to close before returning
  if (url.SchemeIs("mailto")) {
    bool success = XDGEmail(url.spec(), false);
    std::move(callback).Run(success ? "" : "Failed to open path");
  } else {
    bool success = XDGOpen(url.spec(), false, platform_util::OpenCallback());
    std::move(callback).Run(success ? "" : "Failed to open path");
  }
}

bool MoveItemToTrash(const base::FilePath& full_path, bool delete_on_fail) {
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

  return XDGUtil(argv, true, platform_util::OpenCallback());
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
  return base::Environment::Create()->GetVar("CHROME_DESKTOP", setme);
}

}  // namespace platform_util
