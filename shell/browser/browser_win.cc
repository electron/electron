// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

// must come before other includes. fixes bad #defines from <shlwapi.h>.
#include "base/win/shlwapi.h"  // NOLINT(build/include_order)

#include <windows.h>  // NOLINT(build/include_order)

#include <atlbase.h>   // NOLINT(build/include_order)
#include <shlobj.h>    // NOLINT(build/include_order)
#include <shobjidl.h>  // NOLINT(build/include_order)

#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "chrome/browser/icon_manager.h"
#include "electron/electron_version.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/ui/message_box.h"
#include "shell/browser/ui/win/jump_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/skia_util.h"
#include "ui/events/keycodes/keyboard_code_conversion_win.h"

namespace electron {

namespace {

BOOL CALLBACK WindowsEnumerationHandler(HWND hwnd, LPARAM param) {
  DWORD target_process_id = *reinterpret_cast<DWORD*>(param);
  DWORD process_id = 0;

  GetWindowThreadProcessId(hwnd, &process_id);
  if (process_id == target_process_id) {
    SetFocus(hwnd);
    return FALSE;
  }

  return TRUE;
}

bool GetProcessExecPath(base::string16* exe) {
  base::FilePath path;
  if (!base::PathService::Get(base::FILE_EXE, &path)) {
    return false;
  }
  *exe = path.value();
  return true;
}

bool GetProtocolLaunchPath(gin::Arguments* args, base::string16* exe) {
  if (!args->GetNext(exe) && !GetProcessExecPath(exe)) {
    return false;
  }

  // Read in optional args arg
  std::vector<base::string16> launch_args;
  if (args->GetNext(&launch_args) && !launch_args.empty())
    *exe = base::StringPrintf(L"\"%ls\" %ls \"%%1\"", exe->c_str(),
                              base::JoinString(launch_args, L" ").c_str());
  else
    *exe = base::StringPrintf(L"\"%ls\" \"%%1\"", exe->c_str());
  return true;
}

// Windows treats a given scheme as an Internet scheme only if its registry
// entry has a "URL Protocol" key. Check this, otherwise we allow ProgIDs to be
// used as custom protocols which leads to security bugs.
bool IsValidCustomProtocol(const base::string16& scheme) {
  if (scheme.empty())
    return false;
  base::win::RegKey cmd_key(HKEY_CLASSES_ROOT, scheme.c_str(), KEY_QUERY_VALUE);
  return cmd_key.Valid() && cmd_key.HasValue(L"URL Protocol");
}

// Helper for GetApplicationInfoForProtocol().
// takes in an assoc_str
// (https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/ne-shlwapi-assocstr)
// and returns the application name, icon and path that handles the protocol.
//
// Windows 8 introduced a new protocol->executable binding system which cannot
// be retrieved in the HKCR registry subkey method implemented below. We call
// AssocQueryString with the new Win8-only flag ASSOCF_IS_PROTOCOL instead.
base::string16 GetAppInfoHelperForProtocol(ASSOCSTR assoc_str,
                                           const GURL& url) {
  const base::string16 url_scheme = base::ASCIIToUTF16(url.scheme());
  if (!IsValidCustomProtocol(url_scheme))
    return base::string16();

  wchar_t out_buffer[1024];
  DWORD buffer_size = base::size(out_buffer);
  HRESULT hr =
      AssocQueryString(ASSOCF_IS_PROTOCOL, assoc_str, url_scheme.c_str(), NULL,
                       out_buffer, &buffer_size);
  if (FAILED(hr)) {
    DLOG(WARNING) << "AssocQueryString failed!";
    return base::string16();
  }
  return base::string16(out_buffer);
}

void OnIconDataAvailable(const base::FilePath& app_path,
                         const base::string16& app_display_name,
                         gin_helper::Promise<gin_helper::Dictionary> promise,
                         gfx::Image icon) {
  if (!icon.IsEmpty()) {
    v8::HandleScope scope(promise.isolate());
    gin_helper::Dictionary dict =
        gin::Dictionary::CreateEmpty(promise.isolate());

    dict.Set("path", app_path);
    dict.Set("name", app_display_name);
    dict.Set("icon", icon);
    promise.Resolve(dict);
  } else {
    promise.RejectWithErrorMessage("Failed to get file icon.");
  }
}

base::string16 GetAppDisplayNameForProtocol(const GURL& url) {
  return GetAppInfoHelperForProtocol(ASSOCSTR_FRIENDLYAPPNAME, url);
}

base::string16 GetAppPathForProtocol(const GURL& url) {
  return GetAppInfoHelperForProtocol(ASSOCSTR_EXECUTABLE, url);
}

base::string16 GetAppForProtocolUsingRegistry(const GURL& url) {
  const base::string16 url_scheme = base::ASCIIToUTF16(url.scheme());
  if (!IsValidCustomProtocol(url_scheme))
    return base::string16();

  // First, try and extract the application's display name.
  base::string16 command_to_launch;
  base::win::RegKey cmd_key_name(HKEY_CLASSES_ROOT, url_scheme.c_str(),
                                 KEY_READ);
  if (cmd_key_name.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS &&
      !command_to_launch.empty()) {
    return command_to_launch;
  }

  // Otherwise, parse the command line in the registry, and return the basename
  // of the program path if it exists.
  const base::string16 cmd_key_path = url_scheme + L"\\shell\\open\\command";
  base::win::RegKey cmd_key_exe(HKEY_CLASSES_ROOT, cmd_key_path.c_str(),
                                KEY_READ);
  if (cmd_key_exe.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS) {
    base::CommandLine command_line(
        base::CommandLine::FromString(command_to_launch));
    return command_line.GetProgram().BaseName().value();
  }

  return base::string16();
}

bool FormatCommandLineString(base::string16* exe,
                             const std::vector<base::string16>& launch_args) {
  if (exe->empty() && !GetProcessExecPath(exe)) {
    return false;
  }

  if (!launch_args.empty()) {
    *exe = base::StringPrintf(L"%ls %ls", exe->c_str(),
                              base::JoinString(launch_args, L" ").c_str());
  }

  return true;
}

// Helper for GetLoginItemSettings().
// iterates over all the entries in a windows registry path and returns
// a list of launchItem with matching paths to our application.
// if a launchItem with a matching path also has a matching entry within the
// startup_approved_key_path, set executable_will_launch_at_login to be `true`
std::vector<Browser::LaunchItem> GetLoginItemSettingsHelper(
    base::win::RegistryValueIterator* it,
    boolean* executable_will_launch_at_login,
    base::string16 scope,
    const Browser::LoginItemSettings& options) {
  std::vector<Browser::LaunchItem> launch_items;

  base::FilePath lookup_exe_path;
  if (options.path.empty()) {
    base::string16 process_exe_path;
    GetProcessExecPath(&process_exe_path);
    lookup_exe_path =
        base::CommandLine::FromString(process_exe_path).GetProgram();
  } else {
    lookup_exe_path = base::CommandLine::FromString(options.path).GetProgram();
  }

  if (!lookup_exe_path.empty()) {
    while (it->Valid()) {
      base::CommandLine registry_launch_cmd =
          base::CommandLine::FromString(it->Value());
      base::FilePath registry_launch_path = registry_launch_cmd.GetProgram();
      bool exe_match = base::FilePath::CompareEqualIgnoreCase(
          lookup_exe_path.value(), registry_launch_path.value());

      // add launch item to vector if it has a matching path (case-insensitive)
      if (exe_match) {
        Browser::LaunchItem launch_item;
        launch_item.name = it->Name();
        launch_item.path = registry_launch_path.value();
        launch_item.args = registry_launch_cmd.GetArgs();
        launch_item.scope = scope;
        launch_item.enabled = true;

        // attempt to update launch_item.enabled if there is a matching key
        // value entry in the StartupApproved registry
        HKEY hkey;
        // StartupApproved registry path
        LPCTSTR path = TEXT(
            "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApp"
            "roved\\Run");
        LONG res;
        if (scope == L"user") {
          res =
              RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &hkey);
        } else {
          res =
              RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0, KEY_QUERY_VALUE, &hkey);
        }
        if (res == ERROR_SUCCESS) {
          DWORD type, size;
          wchar_t startup_binary[12];
          LONG result =
              RegQueryValueEx(hkey, it->Name(), nullptr, &type,
                              reinterpret_cast<BYTE*>(&startup_binary),
                              &(size = sizeof(startup_binary)));
          if (result == ERROR_SUCCESS) {
            if (type == REG_BINARY) {
              // any other binary other than this indicates that the program is
              // not set to launch at login
              wchar_t binary_accepted[12] = {0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};
              wchar_t binary_accepted_alt[12] = {0x02, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00,
                                                 0x00, 0x00, 0x00, 0x00};
              std::string reg_binary(reinterpret_cast<char*>(binary_accepted));
              std::string reg_binary_alt(
                  reinterpret_cast<char*>(binary_accepted_alt));
              std::string reg_startup_binary(
                  reinterpret_cast<char*>(startup_binary));
              launch_item.enabled = (reg_startup_binary == reg_binary) ||
                                    (reg_startup_binary == reg_binary_alt);
            }
          }
        }

        *executable_will_launch_at_login =
            *executable_will_launch_at_login || launch_item.enabled;
        launch_items.push_back(launch_item);
      }
      it->operator++();
    }
  }
  return launch_items;
}

std::unique_ptr<FileVersionInfo> FetchFileVersionInfo() {
  base::FilePath path;

  if (base::PathService::Get(base::FILE_EXE, &path)) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    return FileVersionInfo::CreateFileVersionInfo(path);
  }
  return std::unique_ptr<FileVersionInfo>();
}

}  // namespace

Browser::UserTask::UserTask() = default;
Browser::UserTask::UserTask(const UserTask&) = default;
Browser::UserTask::~UserTask() = default;

void Browser::Focus(gin::Arguments* args) {
  // On Windows we just focus on the first window found for this process.
  DWORD pid = GetCurrentProcessId();
  EnumWindows(&WindowsEnumerationHandler, reinterpret_cast<LPARAM>(&pid));
}

void GetFileIcon(const base::FilePath& path,
                 v8::Isolate* isolate,
                 base::CancelableTaskTracker* cancelable_task_tracker_,
                 const base::string16 app_display_name,
                 gin_helper::Promise<gin_helper::Dictionary> promise) {
  base::FilePath normalized_path = path.NormalizePathSeparators();
  IconLoader::IconSize icon_size = IconLoader::IconSize::LARGE;

  auto* icon_manager = ElectronBrowserMainParts::Get()->GetIconManager();
  gfx::Image* icon =
      icon_manager->LookupIconFromFilepath(normalized_path, icon_size);
  if (icon) {
    gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("icon", *icon);
    dict.Set("name", app_display_name);
    dict.Set("path", normalized_path);
    promise.Resolve(dict);
  } else {
    icon_manager->LoadIcon(normalized_path, icon_size,
                           base::BindOnce(&OnIconDataAvailable, normalized_path,
                                          app_display_name, std::move(promise)),
                           cancelable_task_tracker_);
  }
}

void GetApplicationInfoForProtocolUsingRegistry(
    v8::Isolate* isolate,
    const GURL& url,
    gin_helper::Promise<gin_helper::Dictionary> promise,
    base::CancelableTaskTracker* cancelable_task_tracker_) {
  base::FilePath app_path;

  const base::string16 url_scheme = base::ASCIIToUTF16(url.scheme());
  if (!IsValidCustomProtocol(url_scheme)) {
    promise.RejectWithErrorMessage("invalid url_scheme");
    return;
  }
  base::string16 command_to_launch;
  const base::string16 cmd_key_path = url_scheme + L"\\shell\\open\\command";
  base::win::RegKey cmd_key_exe(HKEY_CLASSES_ROOT, cmd_key_path.c_str(),
                                KEY_READ);
  if (cmd_key_exe.ReadValue(NULL, &command_to_launch) == ERROR_SUCCESS) {
    base::CommandLine command_line(
        base::CommandLine::FromString(command_to_launch));
    app_path = command_line.GetProgram();
  } else {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return;
  }
  const base::string16 app_display_name = GetAppForProtocolUsingRegistry(url);

  if (app_display_name.empty()) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve application display name");
    return;
  }
  GetFileIcon(app_path, isolate, cancelable_task_tracker_, app_display_name,
              std::move(promise));
}

// resolves `Promise<Object>` - Resolve with an object containing the following:
// * `icon` NativeImage - the display icon of the app handling the protocol.
// * `path` String  - installation path of the app handling the protocol.
// * `name` String - display name of the app handling the protocol.
void GetApplicationInfoForProtocolUsingAssocQuery(
    v8::Isolate* isolate,
    const GURL& url,
    gin_helper::Promise<gin_helper::Dictionary> promise,
    base::CancelableTaskTracker* cancelable_task_tracker_) {
  base::string16 app_path = GetAppPathForProtocol(url);

  if (app_path.empty()) {
    promise.RejectWithErrorMessage(
        "Unable to retrieve installation path to app");
    return;
  }

  base::string16 app_display_name = GetAppDisplayNameForProtocol(url);

  if (app_display_name.empty()) {
    promise.RejectWithErrorMessage("Unable to retrieve display name of app");
    return;
  }

  base::FilePath app_path_file_path = base::FilePath(app_path);
  GetFileIcon(app_path_file_path, isolate, cancelable_task_tracker_,
              app_display_name, std::move(promise));
}

void Browser::AddRecentDocument(const base::FilePath& path) {
  CComPtr<IShellItem> item;
  HRESULT hr = SHCreateItemFromParsingName(path.value().c_str(), NULL,
                                           IID_PPV_ARGS(&item));
  if (SUCCEEDED(hr)) {
    SHARDAPPIDINFO info;
    info.psi = item;
    info.pszAppID = GetAppUserModelID();
    SHAddToRecentDocs(SHARD_APPIDINFO, &info);
  }
}

void Browser::ClearRecentDocuments() {
  SHAddToRecentDocs(SHARD_APPIDINFO, nullptr);
}

void Browser::SetAppUserModelID(const base::string16& name) {
  electron::SetAppUserModelID(name);
}

bool Browser::SetUserTasks(const std::vector<UserTask>& tasks) {
  JumpList jump_list(GetAppUserModelID());
  if (!jump_list.Begin())
    return false;

  JumpListCategory category;
  category.type = JumpListCategory::Type::kTasks;
  category.items.reserve(tasks.size());
  JumpListItem item;
  item.type = JumpListItem::Type::kTask;
  for (const auto& task : tasks) {
    item.title = task.title;
    item.path = task.program;
    item.arguments = task.arguments;
    item.icon_path = task.icon_path;
    item.icon_index = task.icon_index;
    item.description = task.description;
    item.working_dir = task.working_dir;
    category.items.push_back(item);
  }

  jump_list.AppendCategory(category);
  return jump_list.Commit();
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            gin::Arguments* args) {
  if (protocol.empty())
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::string16 keyPath = L"Software\\Classes\\";

  // Command Key
  base::string16 wprotocol = base::UTF8ToUTF16(protocol);
  base::string16 shellPath = wprotocol + L"\\shell";
  base::string16 cmdPath = keyPath + shellPath + L"\\open\\command";

  base::win::RegKey classesKey;
  base::win::RegKey commandKey;

  if (FAILED(classesKey.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Classes key doesn't exist, that's concerning, but I guess
    // we're not the default handler
    return true;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't even exist, we can confirm that it is not set
    return true;

  base::string16 keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return true;

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  if (keyVal == exe) {
    // Let's kill the key
    if (FAILED(classesKey.DeleteKey(shellPath.c_str())))
      return false;

    // Let's clean up after ourselves
    base::win::RegKey protocolKey;
    base::string16 protocolPath = keyPath + wprotocol;

    if (SUCCEEDED(
            protocolKey.Open(root, protocolPath.c_str(), KEY_ALL_ACCESS))) {
      protocolKey.DeleteValue(L"URL Protocol");

      // Overwrite the default value to be empty, we can't delete it right away
      protocolKey.WriteValue(L"", L"");
      protocolKey.DeleteValue(L"");
    }

    // If now empty, delete the whole key
    classesKey.DeleteEmptyKey(wprotocol.c_str());

    return true;
  } else {
    return true;
  }
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol,
                                         gin::Arguments* args) {
  // HKEY_CLASSES_ROOT
  //    $PROTOCOL
  //       (Default) = "URL:$NAME"
  //       URL Protocol = ""
  //       shell
  //          open
  //             command
  //                (Default) = "$COMMAND" "%1"
  //
  // However, the "HKEY_CLASSES_ROOT" key can only be written by the
  // Administrator user. So, we instead write to "HKEY_CURRENT_USER\
  // Software\Classes", which is inherited by "HKEY_CLASSES_ROOT"
  // anyway, and can be written by unprivileged users.

  if (protocol.empty())
    return false;

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::string16 keyPath = base::UTF8ToUTF16("Software\\Classes\\" + protocol);
  base::string16 urlDecl = base::UTF8ToUTF16("URL:" + protocol);

  // Command Key
  base::string16 cmdPath = keyPath + L"\\shell\\open\\command";

  // Write information to registry
  base::win::RegKey key(root, keyPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(key.WriteValue(L"URL Protocol", L"")) ||
      FAILED(key.WriteValue(L"", urlDecl.c_str())))
    return false;

  base::win::RegKey commandKey(root, cmdPath.c_str(), KEY_ALL_ACCESS);
  if (FAILED(commandKey.WriteValue(L"", exe.c_str())))
    return false;

  return true;
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol,
                                      gin::Arguments* args) {
  if (protocol.empty())
    return false;

  base::string16 exe;
  if (!GetProtocolLaunchPath(args, &exe))
    return false;

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  base::string16 keyPath = base::UTF8ToUTF16("Software\\Classes\\" + protocol);

  // Command Key
  base::string16 cmdPath = keyPath + L"\\shell\\open\\command";

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  base::string16 keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return false;

  // Default value is the same as current file path
  return keyVal == exe;
}

base::string16 Browser::GetApplicationNameForProtocol(const GURL& url) {
  // Windows 8 or above has a new protocol association query.
  if (base::win::GetVersion() >= base::win::Version::WIN8) {
    base::string16 application_name = GetAppDisplayNameForProtocol(url);
    if (!application_name.empty())
      return application_name;
  }

  return GetAppForProtocolUsingRegistry(url);
}

v8::Local<v8::Promise> Browser::GetApplicationInfoForProtocol(
    v8::Isolate* isolate,
    const GURL& url) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Windows 8 or above has a new protocol association query.
  if (base::win::GetVersion() >= base::win::Version::WIN8) {
    GetApplicationInfoForProtocolUsingAssocQuery(
        isolate, url, std::move(promise), &cancelable_task_tracker_);
    return handle;
  }

  GetApplicationInfoForProtocolUsingRegistry(isolate, url, std::move(promise),
                                             &cancelable_task_tracker_);
  return handle;
}

bool Browser::SetBadgeCount(int count) {
  return false;
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
  base::string16 key_path =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, key_path.c_str(), KEY_ALL_ACCESS);

  base::string16 startup_approved_key_path =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved"
      L"\\Run";
  base::win::RegKey startup_approved_key(
      HKEY_CURRENT_USER, startup_approved_key_path.c_str(), KEY_ALL_ACCESS);
  PCWSTR key_name =
      !settings.name.empty() ? settings.name.c_str() : GetAppUserModelID();

  if (settings.open_at_login) {
    base::string16 exe = settings.path;
    if (FormatCommandLineString(&exe, settings.args)) {
      key.WriteValue(key_name, exe.c_str());

      if (settings.enabled) {
        startup_approved_key.DeleteValue(key_name);
      } else {
        HKEY hard_key;
        LPCTSTR path = TEXT(
            "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApp"
            "roved\\Run");
        LONG res =
            RegOpenKeyEx(HKEY_CURRENT_USER, path, 0, KEY_ALL_ACCESS, &hard_key);

        if (res == ERROR_SUCCESS) {
          UCHAR disable_startup_binary[] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
          RegSetValueEx(hard_key, key_name, 0, REG_BINARY,
                        reinterpret_cast<const BYTE*>(disable_startup_binary),
                        sizeof(disable_startup_binary));
        }
      }
    }
  } else {
    // if open at login is false, delete both values
    startup_approved_key.DeleteValue(key_name);
    key.DeleteValue(key_name);
  }
}

Browser::LoginItemSettings Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  LoginItemSettings settings;
  base::string16 keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);
  base::string16 keyVal;

  // keep old openAtLogin behaviour
  if (!FAILED(key.ReadValue(GetAppUserModelID(), &keyVal))) {
    base::string16 exe = options.path;
    if (FormatCommandLineString(&exe, options.args)) {
      settings.open_at_login = keyVal == exe;
    }
  }

  // iterate over current user and machine registries and populate launch items
  // if there exists a launch entry with property enabled=='true',
  // set executable_will_launch_at_login to 'true'.
  boolean executable_will_launch_at_login = false;
  std::vector<Browser::LaunchItem> launch_items;
  base::win::RegistryValueIterator hkcu_iterator(HKEY_CURRENT_USER,
                                                 keyPath.c_str());
  base::win::RegistryValueIterator hklm_iterator(HKEY_LOCAL_MACHINE,
                                                 keyPath.c_str());

  launch_items = GetLoginItemSettingsHelper(
      &hkcu_iterator, &executable_will_launch_at_login, L"user", options);
  std::vector<Browser::LaunchItem> launch_items_hklm =
      GetLoginItemSettingsHelper(&hklm_iterator,
                                 &executable_will_launch_at_login, L"machine",
                                 options);
  launch_items.insert(launch_items.end(), launch_items_hklm.begin(),
                      launch_items_hklm.end());

  settings.executable_will_launch_at_login = executable_will_launch_at_login;
  settings.launch_items = launch_items;
  return settings;
}

PCWSTR Browser::GetAppUserModelID() {
  return GetRawAppUserModelID();
}

std::string Browser::GetExecutableFileVersion() const {
  base::FilePath path;
  if (base::PathService::Get(base::FILE_EXE, &path)) {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    std::unique_ptr<FileVersionInfo> version_info = FetchFileVersionInfo();
    return base::UTF16ToUTF8(version_info->product_version());
  }

  return ELECTRON_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  return GetApplicationName();
}

bool Browser::IsEmojiPanelSupported() {
  // emoji picker is supported on Windows 10's Spring 2018 update & above.
  return base::win::GetVersion() >= base::win::Version::WIN10_RS4;
}

void Browser::ShowEmojiPanel() {
  // This sends Windows Key + '.' (both keydown and keyup events).
  // "SendInput" is used because Windows needs to receive these events and
  // open the Emoji picker.
  INPUT input[4] = {};
  input[0].type = INPUT_KEYBOARD;
  input[0].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_COMMAND);
  input[1].type = INPUT_KEYBOARD;
  input[1].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_OEM_PERIOD);

  input[2].type = INPUT_KEYBOARD;
  input[2].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_COMMAND);
  input[2].ki.dwFlags |= KEYEVENTF_KEYUP;
  input[3].type = INPUT_KEYBOARD;
  input[3].ki.wVk = ui::WindowsKeyCodeForKeyboardCode(ui::VKEY_OEM_PERIOD);
  input[3].ki.dwFlags |= KEYEVENTF_KEYUP;
  ::SendInput(4, input, sizeof(INPUT));
}

void Browser::ShowAboutPanel() {
  base::Value dict(base::Value::Type::DICTIONARY);
  std::string aboutMessage = "";
  gfx::ImageSkia image;

  // grab defaults from Windows .EXE file
  std::unique_ptr<FileVersionInfo> exe_info = FetchFileVersionInfo();
  dict.SetStringKey("applicationName", exe_info->file_description());
  dict.SetStringKey("applicationVersion", exe_info->product_version());

  if (about_panel_options_.is_dict()) {
    dict.MergeDictionary(&about_panel_options_);
  }

  std::vector<std::string> stringOptions = {
      "applicationName", "applicationVersion", "copyright", "credits"};

  const std::string* str;
  for (std::string opt : stringOptions) {
    if ((str = dict.FindStringKey(opt))) {
      aboutMessage.append(*str).append("\r\n");
    }
  }

  if ((str = dict.FindStringKey("iconPath"))) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(*str);
    electron::util::PopulateImageSkiaRepsFromPath(&image, path);
  }

  electron::MessageBoxSettings settings = {};
  settings.message = aboutMessage;
  settings.icon = image;
  settings.type = electron::MessageBoxType::kInformation;
  electron::ShowMessageBoxSync(settings);
}

void Browser::SetAboutPanelOptions(base::DictionaryValue options) {
  about_panel_options_ = std::move(options);
}

}  // namespace electron
