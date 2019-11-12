// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/browser.h"

#include <windows.h>  // windows.h must be included first

#include <atlbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/registry.h"
#include "base/win/win_util.h"
#include "base/win/windows_version.h"
#include "electron/electron_version.h"
#include "shell/browser/ui/message_box.h"
#include "shell/browser/ui/win/jump_list.h"
#include "shell/common/application_info.h"
#include "shell/common/gin_helper/arguments.h"
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
    LOG(ERROR) << "Error getting app exe path";
    return false;
  }
  *exe = path.value();
  return true;
}

bool GetProtocolLaunchPath(gin_helper::Arguments* args, base::string16* exe) {
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

// Windows 8 introduced a new protocol->executable binding system which cannot
// be retrieved in the HKCR registry subkey method implemented below. We call
// AssocQueryString with the new Win8-only flag ASSOCF_IS_PROTOCOL instead.
base::string16 GetAppForProtocolUsingAssocQuery(const GURL& url) {
  const base::string16 url_scheme = base::ASCIIToUTF16(url.scheme());
  if (!IsValidCustomProtocol(url_scheme))
    return base::string16();

  // Query AssocQueryString for a human-readable description of the program
  // that will be invoked given the provided URL spec. This is used only to
  // populate the external protocol dialog box the user sees when invoking
  // an unknown external protocol.
  wchar_t out_buffer[1024];
  DWORD buffer_size = base::size(out_buffer);
  HRESULT hr =
      AssocQueryString(ASSOCF_IS_PROTOCOL, ASSOCSTR_FRIENDLYAPPNAME,
                       url_scheme.c_str(), NULL, out_buffer, &buffer_size);
  if (FAILED(hr)) {
    DLOG(WARNING) << "AssocQueryString failed!";
    return base::string16();
  }
  return base::string16(out_buffer);
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

void Browser::Focus() {
  // On Windows we just focus on the first window found for this process.
  DWORD pid = GetCurrentProcessId();
  EnumWindows(&WindowsEnumerationHandler, reinterpret_cast<LPARAM>(&pid));
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
  category.type = JumpListCategory::Type::TASKS;
  category.items.reserve(tasks.size());
  JumpListItem item;
  item.type = JumpListItem::Type::TASK;
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
                                            gin_helper::Arguments* args) {
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
                                         gin_helper::Arguments* args) {
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
                                      gin_helper::Arguments* args) {
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
    base::string16 application_name = GetAppForProtocolUsingAssocQuery(url);
    if (!application_name.empty())
      return application_name;
  }

  return GetAppForProtocolUsingRegistry(url);
}

bool Browser::SetBadgeCount(int count) {
  return false;
}

void Browser::SetLoginItemSettings(LoginItemSettings settings) {
  base::string16 keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);

  if (settings.open_at_login) {
    base::string16 exe = settings.path;
    if (FormatCommandLineString(&exe, settings.args)) {
      key.WriteValue(GetAppUserModelID(), exe.c_str());
    }
  } else {
    key.DeleteValue(GetAppUserModelID());
  }
}

Browser::LoginItemSettings Browser::GetLoginItemSettings(
    const LoginItemSettings& options) {
  LoginItemSettings settings;
  base::string16 keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  base::win::RegKey key(HKEY_CURRENT_USER, keyPath.c_str(), KEY_ALL_ACCESS);
  base::string16 keyVal;

  if (!FAILED(key.ReadValue(GetAppUserModelID(), &keyVal))) {
    base::string16 exe = options.path;
    if (FormatCommandLineString(&exe, options.args)) {
      settings.open_at_login = keyVal == exe;
    }
  }

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
