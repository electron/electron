// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/platform_util.h"

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
#define ELECTRON_DEFAULT_TRASH "gio"

namespace {

bool XDGUtilV(const std::vector<std::string>& argv, const bool wait_for_exit) {
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

  if (wait_for_exit) {
    int exit_code = -1;
    if (!process.WaitForExit(&exit_code))
      return false;
    return (exit_code == 0);
  }

  base::EnsureProcessGetsReaped(std::move(process));
  return true;
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

const char* NextLine(const char* it) {
  while (*it && *it++ != '\n') {
  }
  return it;
}

// Parses a line from /etc/os-release.
// `input` must be a null-terminated string.
// Upon return `name` and `value` contain the parsed variable. When parsing
// fails, they contain an empty string. In all cases the function returns a
// pointer to the next line, or to the terminating null character.
//
// Examples:
//
//      |input               |name      |value
//      |--------------------|----------|----------
//      |VAR=one\ two        |VAR       |one two
//      |VAR=a b c           |          |
//      |VAR=" a b c"        |VAR       | a b c
//      |=                   |          |
//      |VAR=""              |VAR       |
//      | VAR=abc            |VAR       |abc
//      |VAR = abc           |          |
//      |#VAR=abc            |          |
//
const char* ParseOsReleaseVariable(const char* input,
                                   std::string* name,
                                   std::string* value) {
  name->clear();
  value->clear();

  // skip leading whitespace
  while (isspace(*input))
    ++input;

  // ignore comments
  if (*input == '#')
    return NextLine(input);

  std::string tempName, tempValue;

  // parse the variable name
  auto* it = input;
  for (;; ++it) {
    auto c = *it;
    if (!c || isspace(c))
      return NextLine(it);
    if (c == '=')
      break;
  }
  tempName.assign(input, it);
  ++it;  // skip over '='

  // parse the value
  char quote = (*it == '"') ? '"' : 0;
  if (quote)
    ++it;

  for (;; ++it) {
    auto c = *it;

    if (!quote) {
      // non-quoted string doesn't need a terminator
      if (!c || c == '\n')
        break;
      // but it cannot contain unescaped spaces
      if (isspace(c))
        return NextLine(it);
    } else {
      // quoted string must end with quote
      if (!c)
        return it;
      if (c == quote) {
        ++it;
        break;
      }
    }

    if (c == '\\') {
      c = *++it;
      if (!c)
        return it;
    }

    tempValue.push_back(c);
  }

  *name = move(tempName);
  *value = move(tempValue);
  return NextLine(it);
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

  return XDGOpen(dir.value(), false);
}

bool OpenItem(const base::FilePath& full_path) {
  return XDGOpen(full_path.value(), false);
}

bool OpenExternal(const GURL& url, const OpenExternalOptions& options) {
  // Don't wait for exit, since we don't want to wait for the browser/email
  // client window to close before returning
  if (url.SchemeIs("mailto"))
    return XDGEmail(url.spec(), false);
  else
    return XDGOpen(url.spec(), false);
}

void OpenExternal(const GURL& url,
                  const OpenExternalOptions& options,
                  const OpenExternalCallback& callback) {
  // TODO(gabriel): Implement async open if callback is specified
  callback.Run(OpenExternal(url, options) ? "" : "Failed to open");
}

bool MoveItemToTrash(const base::FilePath& full_path) {
  std::string trash;
  if (getenv(ELECTRON_TRASH) != NULL) {
    trash = getenv(ELECTRON_TRASH);
  } else {
    // Determine desktop environment and set accordingly.
    std::unique_ptr<base::Environment> env(base::Environment::Create());
    base::nix::DesktopEnvironment desktop_env(
        base::nix::GetDesktopEnvironment(env.get()));
    if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE4 ||
        desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE5) {
      trash = "kioclient5";
    } else if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE3) {
      trash = "kioclient";
    } else {
      trash = ELECTRON_DEFAULT_TRASH;
    }
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
  } else if (trash.compare("gvfs-trash") == 0) {
    // retain support for deprecated gvfs-trash
    argv.push_back("gvfs-trash");
    argv.push_back(full_path.value());
  } else {
    argv.push_back(ELECTRON_DEFAULT_TRASH);
    argv.push_back("trash");
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

std::string GetLinuxDistro() {
  // Reference: https://www.freedesktop.org/software/systemd/man/os-release.html

  auto path1 = base::FilePath::FromUTF8Unsafe("/etc/os-release");
  auto path2 = base::FilePath::FromUTF8Unsafe("/usr/lib/os-release");

  std::string buffer;
  if (!base::ReadFileToString(path1, &buffer)) {
    base::ReadFileToString(path2, &buffer);
  }

  if (!buffer.empty()) {
    std::string name, value;

    const char* it = buffer.c_str();
    while (*it) {
      it = ParseOsReleaseVariable(it, &name, &value);
      if (name == "PRETTY_NAME") {
        return value;
      }
    }
  }

  return "Unknown";
}

}  // namespace platform_util
