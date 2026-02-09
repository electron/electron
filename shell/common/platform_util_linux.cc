// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/platform_util.h"

#include <fcntl.h>

#include <stdio.h>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

#include <gdk/gdk.h>

#include "base/cancelable_callback.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/nix/xdg_util.h"
#include "base/no_destructor.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/run_loop.h"
#include "base/strings/escape.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "base/types/expected.h"
#include "components/dbus/thread_linux/dbus_thread_linux.h"
#include "components/dbus/utils/call_method.h"
#include "components/dbus/utils/check_for_service_and_start.h"
#include "components/dbus/xdg/request.h"
#include "content/public/browser/browser_thread.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_proxy.h"

#include "shell/common/platform_util_internal.h"
#include "url/gurl.h"

#define ELECTRON_TRASH "ELECTRON_TRASH"

namespace platform_util {
void OpenFolder(const base::FilePath& full_path);
}

namespace {

const char kFreedesktopFileManagerName[] = "org.freedesktop.FileManager1";
const char kFreedesktopFileManagerPath[] = "/org/freedesktop/FileManager1";

const char kMethodShowItems[] = "ShowItems";

const char kFreedesktopPortalName[] = "org.freedesktop.portal.Desktop";
const char kFreedesktopPortalPath[] = "/org/freedesktop/portal/desktop";
const char kFreedesktopPortalOpenURI[] = "org.freedesktop.portal.OpenURI";

const char kMethodOpenDirectory[] = "OpenDirectory";
const char kActivationTokenKey[] = "activation_token";

class ShowItemHelper {
 public:
  static ShowItemHelper& GetInstance() {
    static base::NoDestructor<ShowItemHelper> instance;
    return *instance;
  }

  ShowItemHelper() = default;

  ShowItemHelper(const ShowItemHelper&) = delete;
  ShowItemHelper& operator=(const ShowItemHelper&) = delete;

  void ShowItemInFolder(const base::FilePath& full_path) {
    if (!bus_) {
      bus_ = dbus_thread_linux::GetSharedSessionBus();
    }

    if (api_type_.has_value()) {
      ShowItemInFolderOnApiTypeSet(full_path);
      return;
    }

    bool api_availability_check_in_progress = !pending_requests_.empty();
    pending_requests_.push(full_path);
    if (!api_availability_check_in_progress) {
      // Initiate check to determine if portal or the FileManager API should
      // be used. The portal API is always preferred if available.
      dbus_utils::CheckForServiceAndStart(
          bus_.get(), kFreedesktopPortalName,
          base::BindOnce(&ShowItemHelper::CheckPortalRunningResponse,
                         // Unretained is safe, the ShowItemHelper instance is
                         // never destroyed.
                         base::Unretained(this)));
    }
  }

 private:
  enum class ApiType { kNone, kPortal, kFileManager };

  void ShowItemInFolderOnApiTypeSet(const base::FilePath& full_path) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    CHECK(api_type_.has_value());
    switch (*api_type_) {
      case ApiType::kPortal:
        ShowItemUsingPortal(full_path);
        break;
      case ApiType::kFileManager:
        ShowItemUsingFileManager(full_path);
        break;
      case ApiType::kNone:
        OpenParentFolderFallback(full_path);
        break;
    }
  }

  void ProcessPendingRequests() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!bus_) {
      return;
    }

    CHECK(!pending_requests_.empty());
    while (!pending_requests_.empty()) {
      ShowItemInFolderOnApiTypeSet(pending_requests_.front());
      pending_requests_.pop();
    }
  }

  void CheckPortalRunningResponse(std::optional<bool> is_running) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (is_running.value_or(false)) {
      api_type_ = ApiType::kPortal;
      ProcessPendingRequests();
    } else {
      // Portal is unavailable.
      // Check if FileManager is available.
      dbus_utils::CheckForServiceAndStart(
          bus_.get(), kFreedesktopFileManagerName,
          base::BindOnce(&ShowItemHelper::CheckFileManagerRunningResponse,
                         // Unretained is safe, the ShowItemHelper instance is
                         // never destroyed.
                         base::Unretained(this)));
    }
  }

  void CheckFileManagerRunningResponse(std::optional<bool> is_running) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (is_running.value_or(false)) {
      api_type_ = ApiType::kFileManager;
    } else {
      // Neither portal nor FileManager is available.
      api_type_ = ApiType::kNone;
    }
    ProcessPendingRequests();
  }

  void ShowItemUsingPortal(const base::FilePath& full_path) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    CHECK(api_type_.has_value());
    CHECK_EQ(*api_type_, ApiType::kPortal);
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(
            [](const base::FilePath& full_path) {
              base::ScopedFD fd(HANDLE_EINTR(
                  open(full_path.value().c_str(), O_RDONLY | O_CLOEXEC)));
              return fd;
            },
            full_path),
        base::BindOnce(&ShowItemHelper::ShowItemUsingPortalFdOpened,
                       // Unretained is safe, the ShowItemHelper instance is
                       // never destroyed.
                       base::Unretained(this), full_path));
  }

  void ShowItemUsingPortalFdOpened(const base::FilePath& full_path,
                                   base::ScopedFD fd) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!bus_) {
      return;
    }
    if (!fd.is_valid()) {
      // At least open the parent folder, as long as we're not in the unit
      // tests.
      OpenParentFolderFallback(full_path);
      return;
    }
    base::nix::CreateXdgActivationToken(base::BindOnce(
        &ShowItemHelper::ShowItemUsingPortalWithToken,
        // Unretained is safe, the ShowItemHelper instance is never destroyed.
        base::Unretained(this), full_path, std::move(fd)));
  }

  void ShowItemUsingPortalWithToken(const base::FilePath& full_path,
                                    base::ScopedFD fd,
                                    std::string activation_token) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!bus_) {
      return;
    }

    if (!portal_object_proxy_) {
      portal_object_proxy_ = bus_->GetObjectProxy(
          kFreedesktopPortalName, dbus::ObjectPath(kFreedesktopPortalPath));
    }

    dbus_xdg::Dictionary options;
    options[kActivationTokenKey] =
        dbus_utils::Variant::Wrap<"s">(activation_token);
    // In the rare occasion that another request comes in before the response is
    // received, we will end up overwriting this request object with the new one
    // and the response from the first request will not be handled in that case.
    // This should be acceptable as it means the two requests were received too
    // close to each other from the user and the first one was handled on a best
    // effort basis.
    portal_open_directory_request_ = std::make_unique<dbus_xdg::Request>(
        bus_, portal_object_proxy_, kFreedesktopPortalOpenURI,
        kMethodOpenDirectory, std::move(options),
        base::BindOnce(&ShowItemHelper::ShowItemUsingPortalResponse,
                       // Unretained is safe, the ShowItemHelper instance is
                       // never destroyed.
                       base::Unretained(this), full_path),
        std::string(), std::move(fd));
  }

  void ShowItemUsingPortalResponse(
      const base::FilePath& full_path,
      base::expected<dbus_xdg::Dictionary, dbus_xdg::ResponseError> results) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    portal_open_directory_request_.reset();
    if (!results.has_value()) {
      OpenParentFolderFallback(full_path);
    }
  }

  void ShowItemUsingFileManager(const base::FilePath& full_path) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!bus_) {
      return;
    }
    CHECK(api_type_.has_value());
    CHECK_EQ(*api_type_, ApiType::kFileManager);
    if (!file_manager_object_proxy_) {
      file_manager_object_proxy_ =
          bus_->GetObjectProxy(kFreedesktopFileManagerName,
                               dbus::ObjectPath(kFreedesktopFileManagerPath));
    }

    std::vector<std::string> file_to_highlight{"file://" + full_path.value()};
    dbus_utils::CallMethod<"ass", "">(
        file_manager_object_proxy_, kFreedesktopFileManagerName,
        kMethodShowItems,
        base::BindOnce(&ShowItemHelper::ShowItemUsingFileManagerResponse,
                       // Unretained is safe, the ShowItemHelper instance is
                       // never destroyed.
                       base::Unretained(this), full_path),
        std::move(file_to_highlight), /*startup-id=*/"");
  }

  void ShowItemUsingFileManagerResponse(
      const base::FilePath& full_path,
      dbus_utils::CallMethodResultSig<""> response) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (!response.has_value()) {
      // If the bus call fails, at least open the parent folder.
      OpenParentFolderFallback(full_path);
    }
  }

  void OpenParentFolderFallback(const base::FilePath& full_path) {
    platform_util::OpenFolder(full_path.DirName());
  }

  scoped_refptr<dbus::Bus> bus_;

  std::optional<ApiType> api_type_;
  // The proxy objects are owned by `bus_`.
  raw_ptr<dbus::ObjectProxy> portal_object_proxy_ = nullptr;
  raw_ptr<dbus::ObjectProxy> file_manager_object_proxy_ = nullptr;
  std::unique_ptr<dbus_xdg::Request> portal_open_directory_request_;

  // Requests that are queued until the API availability is determined.
  std::queue<base::FilePath> pending_requests_;
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
             const bool focus_launched_process,
             platform_util::OpenCallback callback) {
  base::LaunchOptions options;
  if (focus_launched_process) {
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    base::RepeatingClosure quit_loop = run_loop.QuitClosure();
    base::nix::CreateLaunchOptionsWithXdgActivation(base::BindOnce(
        [](base::RepeatingClosure quit_loop, base::LaunchOptions* options_out,
           base::LaunchOptions options) {
          *options_out = std::move(options);
          std::move(quit_loop).Run();
        },
        std::move(quit_loop), &options));
    run_loop.Run();
  }
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
                 /*focus_launched_process=*/true, std::move(callback));
}

bool XDGEmail(const std::string& email, const bool wait_for_exit) {
  return XDGUtil({"xdg-email", email}, base::FilePath(), wait_for_exit,
                 /*focus_launched_process=*/true,
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
  XDGOpen(full_path.DirName(), full_path.value(), false, std::move(callback));
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
  auto env = base::Environment::Create();

  // find the trash method
  std::string trash = env->GetVar(ELECTRON_TRASH).value_or("");
  if (trash.empty()) {
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

  return XDGUtil(argv, base::FilePath(), true, /*focus_launched_process=*/false,
                 platform_util::OpenCallback());
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
  auto* display = gdk_display_get_default();
  if (!display)
    return;
  gdk_display_beep(display);
}

std::optional<std::string> GetDesktopName() {
  return base::Environment::Create()->GetVar("CHROME_DESKTOP");
}

std::optional<std::string> GetXdgAppId() {
  auto name = GetDesktopName();
  if (!name)
    return {};

  // remove '.desktop' file suffix, if present
  if (std::string_view suffix = ".desktop"; name->ends_with(suffix))
    name->resize(std::size(*name) - std::size(suffix));

  return *name;
}

}  // namespace platform_util
