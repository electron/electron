// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <atlbase.h>
#include <propkey.h>
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/win_util.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "atom/common/atom_version.h"

namespace atom {

namespace {

const wchar_t kAppUserModelIDFormat[] = L"electron.app.$1";

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

}  // namespace

void Browser::Focus() {
  // On Windows we just focus on the first window found for this process.
  DWORD pid = GetCurrentProcessId();
  EnumWindows(&WindowsEnumerationHandler, reinterpret_cast<LPARAM>(&pid));
}

void Browser::AddRecentDocument(const base::FilePath& path) {
  if (base::win::GetVersion() < base::win::VERSION_WIN7)
    return;

  CComPtr<IShellItem> item;
  HRESULT hr = SHCreateItemFromParsingName(
      path.value().c_str(), NULL, IID_PPV_ARGS(&item));
  if (SUCCEEDED(hr)) {
    SHARDAPPIDINFO info;
    info.psi = item;
    info.pszAppID = GetAppUserModelID();
    SHAddToRecentDocs(SHARD_APPIDINFO, &info);
  }
}

void Browser::ClearRecentDocuments() {
  CComPtr<IApplicationDestinations> destinations;
  if (FAILED(destinations.CoCreateInstance(CLSID_ApplicationDestinations,
                                           NULL, CLSCTX_INPROC_SERVER)))
    return;
  if (FAILED(destinations->SetAppID(GetAppUserModelID())))
    return;
  destinations->RemoveAllDestinations();
}

void Browser::SetAppUserModelID(const base::string16& name) {
  app_user_model_id_ = name;
  SetCurrentProcessExplicitAppUserModelID(app_user_model_id_.c_str());
}

void Browser::SetUserTasks(const std::vector<UserTask>& tasks) {
  CComPtr<ICustomDestinationList> destinations;
  if (FAILED(destinations.CoCreateInstance(CLSID_DestinationList)))
    return;
  if (FAILED(destinations->SetAppID(GetAppUserModelID())))
    return;

  // Start a transaction that updates the JumpList of this application.
  UINT max_slots;
  CComPtr<IObjectArray> removed;
  if (FAILED(destinations->BeginList(&max_slots, IID_PPV_ARGS(&removed))))
    return;

  CComPtr<IObjectCollection> collection;
  if (FAILED(collection.CoCreateInstance(CLSID_EnumerableObjectCollection)))
    return;

  for (auto& task : tasks) {
    CComPtr<IShellLink> link;
    if (FAILED(link.CoCreateInstance(CLSID_ShellLink)) ||
        FAILED(link->SetPath(task.program.value().c_str())) ||
        FAILED(link->SetArguments(task.arguments.c_str())) ||
        FAILED(link->SetDescription(task.description.c_str())))
      return;

    if (!task.icon_path.empty() &&
        FAILED(link->SetIconLocation(task.icon_path.value().c_str(),
                                     task.icon_index)))
      return;

    CComQIPtr<IPropertyStore> property_store = link;
    if (!base::win::SetStringValueForPropertyStore(property_store, PKEY_Title,
                                                   task.title.c_str()))
      return;

    if (FAILED(collection->AddObject(link)))
      return;
  }

  // When the list is empty "AddUserTasks" could fail, so we don't check return
  // value for it.
  CComQIPtr<IObjectArray> task_array = collection;
  destinations->AddUserTasks(task_array);
  destinations->CommitList();
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol) {
  if (protocol.empty())
    return false;

  base::FilePath path;
  if (!PathService::Get(base::FILE_EXE, &path)) {
    LOG(ERROR) << "Error getting app exe path";
    return false;
  }

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  std::string keyPathStr = "Software\\Classes\\" + protocol;
  std::wstring keyPath = std::wstring(keyPathStr.begin(), keyPathStr.end());

  // Command Key
  std::string cmdPathStr = keyPathStr + "\\shell\\open\\command";
  std::wstring cmdPath = std::wstring(cmdPathStr.begin(), cmdPathStr.end());

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't even exist, we can confirm that it is not set
    return true;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't even exist, we can confirm that it is not set
    return true;

  std::wstring keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return true;

  std::wstring exePath(path.value());
  std::wstring exe = L"\"" + exePath + L"\" \"%1\"";
  if (keyVal == exe) {
    // Let's kill the key
    if (FAILED(key.DeleteKey(L"shell")))
      return false;

    return true;
  } else {
    return true;
  }
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol) {
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

  base::FilePath path;
  if (!PathService::Get(base::FILE_EXE, &path)) {
    LOG(ERROR) << "Error getting app exe path";
    return false;
  }

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  std::string keyPathStr = "Software\\Classes\\" + protocol;
  std::wstring keyPath = std::wstring(keyPathStr.begin(), keyPathStr.end());
  std::string urlDeclStr = "URL:" + protocol;
  std::wstring urlDecl = std::wstring(urlDeclStr.begin(), urlDeclStr.end());

  // Command Key
  std::string cmdPathStr = keyPathStr + "\\shell\\open\\command";
  std::wstring cmdPath = std::wstring(cmdPathStr.begin(), cmdPathStr.end());

  // Executable Path
  std::wstring exePath(path.value());
  std::wstring exe = L"\"" + exePath + L"\" \"%1\"";

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

bool Browser::IsDefaultProtocolClient(const std::string& protocol) {
  if (protocol.empty())
    return false;

  base::FilePath path;
  if (!PathService::Get(base::FILE_EXE, &path)) {
    LOG(ERROR) << "Error getting app exe path";
    return false;
  }

  // Main Registry Key
  HKEY root = HKEY_CURRENT_USER;
  std::string keyPathStr = "Software\\Classes\\" + protocol;
  std::wstring keyPath = std::wstring(keyPathStr.begin(), keyPathStr.end());

  // Command Key
  std::string cmdPathStr = keyPathStr + "\\shell\\open\\command";
  std::wstring cmdPath = std::wstring(cmdPathStr.begin(), cmdPathStr.end());

  base::win::RegKey key;
  base::win::RegKey commandKey;
  if (FAILED(key.Open(root, keyPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  if (FAILED(commandKey.Open(root, cmdPath.c_str(), KEY_ALL_ACCESS)))
    // Key doesn't exist, we can confirm that it is not set
    return false;

  std::wstring keyVal;
  if (FAILED(commandKey.ReadValue(L"", &keyVal)))
    // Default value not set, we can confirm that it is not set
    return false;

  std::wstring exePath(path.value());
  std::wstring exe = L"\"" + exePath + L"\" \"%1\"";
  if (keyVal == exe) {
    // Default value is the same as current file path
    return true;
  } else {
    return false;
  }
}

PCWSTR Browser::GetAppUserModelID() {
  if (app_user_model_id_.empty()) {
    SetAppUserModelID(base::ReplaceStringPlaceholders(
        kAppUserModelIDFormat, base::UTF8ToUTF16(GetName()), nullptr));
  }

  return app_user_model_id_.c_str();
}

std::string Browser::GetExecutableFileVersion() const {
  base::FilePath path;
  if (PathService::Get(base::FILE_EXE, &path)) {
    scoped_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(path));
    return base::UTF16ToUTF8(version_info->product_version());
  }

  return ATOM_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  base::FilePath path;
  if (PathService::Get(base::FILE_EXE, &path)) {
    scoped_ptr<FileVersionInfo> version_info(
        FileVersionInfo::CreateFileVersionInfo(path));
    return base::UTF16ToUTF8(version_info->product_name());
  }

  return ATOM_PRODUCT_NAME;
}

}  // namespace atom
