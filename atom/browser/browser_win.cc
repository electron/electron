// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <windows.h>  // windows.h must be included first

#include <atlbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "atom/browser/ui/win/jump_list.h"
#include "atom/common/application_info.h"
#include "atom/common/atom_version.h"
#include "atom/common/native_mate_converters/string16_converter.h"
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

namespace atom {

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

bool GetProtocolLaunchPath(mate::Arguments* args, base::string16* exe) {
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
  atom::SetAppUserModelID(name);
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
    category.items.push_back(item);
  }

  jump_list.AppendCategory(category);
  return jump_list.Commit();
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol,
                                            mate::Arguments* args) {
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
                                         mate::Arguments* args) {
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
                                      mate::Arguments* args) {
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
    std::unique_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(path));
    return base::UTF16ToUTF8(version_info->product_version());
  }

  return ATOM_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  return GetApplicationName();
}

}  // namespace atom
