// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <atlbase.h>
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>

#include "base/base_paths.h"
#include "base/file_version_info.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "atom/common/atom_version.h"

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
    info.pszAppID = app_user_model_id_.c_str();
    SHAddToRecentDocs(SHARD_APPIDINFO, &info);
  }
}

void Browser::ClearRecentDocuments() {
  CComPtr<IApplicationDestinations> destinations;
  if (FAILED(destinations.CoCreateInstance(CLSID_ApplicationDestinations,
                                           NULL, CLSCTX_INPROC_SERVER)))
    return;
  if (FAILED(destinations->SetAppID(app_user_model_id_.c_str())))
    return;
  destinations->RemoveAllDestinations();
}

void Browser::SetAppUserModelID(const std::string& name) {
  app_user_model_id_ = base::UTF8ToUTF16(
      base::StringPrintf("atom-shell.app.%s", name));
  SetCurrentProcessExplicitAppUserModelID(app_user_model_id_.c_str());
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

  return "Atom-Shell";
}

}  // namespace atom
