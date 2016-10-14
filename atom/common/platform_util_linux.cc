// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

#include <stdio.h>

#include "base/files/file_util.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "url/gurl.h"

#define ELECTRON_TRASH "ELECTRON_TRASH"
#define ELECTRON_DEFAULT_TRASH "gvfs-trash"

namespace {

bool XDGUtilV(const std::vector<std::string>& argv,
             const bool wait_for_exit) {
  base::LaunchOptions options;
  options.allow_new_privs = true;
  // xdg-open can fall back on mailcap which eventually might plumb through
  // to a command that needs a terminal.  Set the environment variable telling
  // it that we definitely don't have a terminal available and that it should
  // bring up a new terminal if necessary.  See "man mailcap".
  options.environ["MM_NOTTTY"] = "1";

  base::Process process = base::LaunchProcess(argv, options);
  if (!process.IsValid())
    return false;

  if (!wait_for_exit) {
    base::EnsureProcessGetsReaped(process.Pid());
    return true;
  }

  int exit_code = -1;
  if (!process.WaitForExit(&exit_code))
    return false;

  return (exit_code == 0);
}

bool XDGUtil(const std::string& util,
             const std::string& arg,
             const bool wait_for_exit) {
  std::vector<std::string> argv;
  argv.push_back(util);
  argv.push_back(arg);

  return XDGUtilV(argv, wait_for_exit);
}

bool XDGOpen(const std::string& path, const bool wait_for_exit) {
  return XDGUtil("xdg-open", path, wait_for_exit);
}

bool XDGEmail(const std::string& email, const bool wait_for_exit) {
  return XDGUtil("xdg-email", email, wait_for_exit);
}

}  // namespace

namespace platform_util {

// TODO(estade): It would be nice to be able to select the file in the file
// manager, but that probably requires extending xdg-open. For now just
// show the folder.
bool ShowItemInFolder(const base::FilePath& full_path) {
  base::FilePath dir = full_path.DirName();
  if (!base::DirectoryExists(dir))
    return false;

  return XDGOpen(dir.value(), true);
}

bool OpenItem(const base::FilePath& full_path) {
  return XDGOpen(full_path.value(), true);
}

bool openExternal(const GURL& url, bool activate) {
  // Don't wait for exit, since we don't want to wait for the browser/email
  // client window to close before returning
  if (url.SchemeIs("mailto"))
    return XDGEmail(url.spec(), false);
  else
    return XDGOpen(url.spec(), false);
}

bool OpenExternal(const GURL& url, bool activate) {
  return openExternal(url, activate);
}

bool OpenExternal(const GURL& url, bool activate, const OpenExternalCallback& callback) {
  // TODO: Implement async open if callback is specified
  bool opened = openExternal(url, activate);
  callback.Run(opened);
  return opened;
}

bool MoveItemToTrash(const base::FilePath& full_path) {
  std::string trash;
  if (getenv(ELECTRON_TRASH) != NULL) {
    trash = getenv(ELECTRON_TRASH);
  } else {
    trash = ELECTRON_DEFAULT_TRASH;
  }

  std::vector<std::string> argv;

  if (trash.compare("kioclient5") == 0 || trash.compare("kioclient") == 0) {
    argv.push_back(trash);
    argv.push_back("move");
    argv.push_back(full_path.value());
    argv.push_back("trash:/");
  } else if (trash.compare("trash-cli") == 0) {
    argv.push_back("trash-put");
    argv.push_back(full_path.value());
  } else {
    argv.push_back(ELECTRON_DEFAULT_TRASH);
    argv.push_back(full_path.value());
  }
  return XDGUtilV(argv, true);
}

void Beep() {
  // echo '\a' > /dev/console
  FILE* console = fopen("/dev/console", "r");
  if (console == NULL)
    return;
  fprintf(console, "\a");
  fclose(console);
}

}  // namespace platform_util
