// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/platform_util.h"

#include <stdio.h>

#include "base/cancelable_callback.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/nix/xdg_util.h"
#include "base/no_destructor.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/threading/thread_restrictions.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "content/public/browser/browser_thread.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"
#include "shell/common/platform_util_internal.h"
#include "ui/gtk/gtk_util.h"
#include "url/gurl.h"

#define ELECTRON_TRASH "ELECTRON_TRASH"

namespace platform_util {
void OpenFolder(const base::FilePath& full_path);
}

namespace {

const char kFreedesktopFileManagerName[] = "org.freedesktop.FileManager1";
const char kFreedesktopFileManagerPath[] = "/org/freedesktop/FileManager1";

const char kMethodShowItems[] = "ShowItems";

class ShowItemHelper {
 public:
  static ShowItemHelper& GetInstance() {
    static base::NoDestructor<ShowItemHelper> instance;
    return *instance;
  }

  ShowItemHelper() {}

  ShowItemHelper(const ShowItemHelper&) = delete;
  ShowItemHelper& operator=(const ShowItemHelper&) = delete;

  void ShowItemInFolder(const base::FilePath& full_path) {
    if (!bus_) {
      // Sets up the D-Bus connection.
      dbus::Bus::Options bus_options;
      bus_options.bus_type = dbus::Bus::SESSION;
      bus_options.connection_type = dbus::Bus::PRIVATE;
      bus_options.dbus_task_runner = dbus_thread_linux::GetTaskRunner();
      bus_ = base::MakeRefCounted<dbus::Bus>(bus_options);
    }

    if (!filemanager_proxy_) {
      filemanager_proxy_ =
          bus_->GetObjectProxy(kFreedesktopFileManagerName,
                               dbus::ObjectPath(kFreedesktopFileManagerPath));
    }

    dbus::MethodCall show_items_call(kFreedesktopFileManagerName,
                                     kMethodShowItems);
    dbus::MessageWriter writer(&show_items_call);

    writer.AppendArrayOfStrings(
        {"file://" + full_path.value()});  // List of file(s) to highlight.
    writer.AppendString({});               // startup-id

    filemanager_proxy_->CallMethod(
        &show_items_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&ShowItemHelper::ShowItemInFolderResponse,
                       base::Unretained(this), full_path));
  }

 private:
  void ShowItemInFolderResponse(const base::FilePath& full_path,
                                dbus::Response* response) {
    if (response)
      return;

    LOG(ERROR) << "Error calling " << kMethodShowItems;
    // If the FileManager1 call fails, at least open the parent folder.
    platform_util::OpenFolder(full_path.DirName());
  }

  scoped_refptr<dbus::Bus> bus_;
  dbus::ObjectProxy* filemanager_proxy_ = nullptr;
};

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
             const base::FilePath& working_directory,
             const bool wait_for_exit,
             platform_util::OpenCallback callback) {
  base::LaunchOptions options;
  options.current_directory = working_directory;
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
    base::ScopedAllowBaseSyncPrimitivesForTesting
        allow_sync;  // required by WaitForExit
    int exit_code = -1;
    bool success = process.WaitForExit(&exit_code);
    if (!callback.is_null())
      std::move(callback).Run(GetErrorDescription(exit_code));
    return success ? (exit_code == 0) : false;
  }

  base::EnsureProcessGetsReaped(std::move(process));
  return true;
}

bool XDGOpen(const base::FilePath& working_directory,
             const std::string& path,
             const bool wait_for_exit,
             platform_util::OpenCallback callback) {
  return XDGUtil({"xdg-open", path}, working_directory, wait_for_exit,
                 std::move(callback));
}

bool XDGEmail(const std::string& email, const bool wait_for_exit) {
  return XDGUtil({"xdg-email", email}, base::FilePath(), wait_for_exit,
                 platform_util::OpenCallback());
}

}  // namespace

namespace platform_util {

void ShowItemInFolder(const base::FilePath& full_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ShowItemHelper::GetInstance().ShowItemInFolder(full_path);
}

void OpenPath(const base::FilePath& full_path, OpenCallback callback) {
  // This is async, so we don't care about the return value.
  XDGOpen(full_path.DirName(), full_path.value(), true, std::move(callback));
}

void OpenFolder(const base::FilePath& full_path) {
  if (!base::DirectoryExists(full_path))
    return;

  XDGOpen(full_path.DirName(), ".", false, platform_util::OpenCallback());
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
    bool success = XDGOpen(base::FilePath(), url.spec(), false,
                           platform_util::OpenCallback());
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

  return XDGUtil(argv, base::FilePath(), true, platform_util::OpenCallback());
}

namespace internal {

bool PlatformTrashItem(const base::FilePath& full_path, std::string* error) {
  if (!MoveItemToTrash(full_path, false)) {
    // TODO(nornagon): at least include the exit code?
    *error = "Failed to move item to trash";
    return false;
  }
  return true;
}

}  // namespace internal

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
