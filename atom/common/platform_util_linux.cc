// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

#include <stdio.h>

#include "base/files/file_util.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "url/gurl.h"

namespace {

bool XDGUtil(const std::string& util,
             const std::string& arg,
             const bool wait_for_exit) {
  std::vector<std::string> argv;
  argv.push_back(util);
  argv.push_back(arg);

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
void ShowItemInFolder(const base::FilePath& full_path) {
  base::FilePath dir = full_path.DirName();
  if (!base::DirectoryExists(dir))
    return;

  XDGOpen(dir.value(), true);
}

void OpenItem(const base::FilePath& full_path) {
  XDGOpen(full_path.value(), true);
}

bool OpenExternal(const GURL& url, bool activate) {
  // Don't wait for exit, since we don't want to wait for the browser/email
  // client window to close before returning
  if (url.SchemeIs("mailto"))
    return XDGEmail(url.spec(), false);
  else
    return XDGOpen(url.spec(), false);
}

bool MoveItemToTrash(const base::FilePath& full_path) {
  return XDGUtil("gvfs-trash", full_path.value(), true);
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
