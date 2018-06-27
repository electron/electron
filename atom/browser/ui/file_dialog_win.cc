// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/file_dialog.h"

#include <windows.h>  // windows.h must be included first

#include <atlbase.h>  // atlbase.h for CComPtr

#include <shlobj.h>
#include <shobjidl.h>

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/registry.h"

// Used to be linked  by WTL
#pragma comment(lib, "gdi32.lib")

namespace file_dialog {

DialogSettings::DialogSettings() = default;
DialogSettings::DialogSettings(const DialogSettings&) = default;
DialogSettings::~DialogSettings() = default;

namespace {

// Distinguish directories from regular files.
bool IsDirectory(const base::FilePath& path) {
  base::File::Info file_info;
  return base::GetFileInfo(path, &file_info) ? file_info.is_directory
                                             : path.EndsWithSeparator();
}

void ConvertFilters(const Filters& filters,
                    std::vector<std::wstring>* buffer,
                    std::vector<COMDLG_FILTERSPEC>* filterspec) {
  if (filters.empty()) {
    COMDLG_FILTERSPEC spec = {L"All Files (*.*)", L"*.*"};
    filterspec->push_back(spec);
    return;
  }

  buffer->reserve(filters.size() * 2);
  for (size_t i = 0; i < filters.size(); ++i) {
    const Filter& filter = filters[i];

    COMDLG_FILTERSPEC spec;
    buffer->push_back(base::UTF8ToWide(filter.first));
    spec.pszName = buffer->back().c_str();

    std::vector<std::string> extensions(filter.second);
    for (size_t j = 0; j < extensions.size(); ++j)
      extensions[j].insert(0, "*.");
    buffer->push_back(base::UTF8ToWide(base::JoinString(extensions, ";")));
    spec.pszSpec = buffer->back().c_str();

    filterspec->push_back(spec);
  }
}

struct RunState {
  base::Thread* dialog_thread;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner;
};

bool CreateDialogThread(RunState* run_state) {
  auto thread =
      std::make_unique<base::Thread>(ATOM_PRODUCT_NAME "FileDialogThread");
  thread->init_com_with_mta(false);
  if (!thread->Start())
    return false;

  run_state->dialog_thread = thread.release();
  run_state->ui_task_runner = base::ThreadTaskRunnerHandle::Get();
  return true;
}

void RunOpenDialogInNewThread(const RunState& run_state,
                              const DialogSettings& settings,
                              const OpenDialogCallback& callback) {
  std::vector<base::FilePath> paths;
  bool result = ShowOpenDialog(settings, &paths);
  run_state.ui_task_runner->PostTask(FROM_HERE,
                                     base::Bind(callback, result, paths));
  run_state.ui_task_runner->DeleteSoon(FROM_HERE, run_state.dialog_thread);
}

void RunSaveDialogInNewThread(const RunState& run_state,
                              const DialogSettings& settings,
                              const SaveDialogCallback& callback) {
  base::FilePath path;
  bool result = ShowSaveDialog(settings, &path);
  run_state.ui_task_runner->PostTask(FROM_HERE,
                                     base::Bind(callback, result, path));
  run_state.ui_task_runner->DeleteSoon(FROM_HERE, run_state.dialog_thread);
}

}  // namespace

static HRESULT GetFileNameFromShellItem(IShellItem* pShellItem,
                                        SIGDN type,
                                        LPWSTR lpstr,
                                        int cchLength) {
  assert(pShellItem != NULL);

  LPWSTR lpstrName = NULL;
  HRESULT hRet = pShellItem->GetDisplayName(type, &lpstrName);

  if (SUCCEEDED(hRet)) {
    if (wcslen(lpstrName) < cchLength) {
      wcscpy_s(lpstr, cchLength, lpstrName);
    } else {
      assert(FALSE);
      hRet = DISP_E_BUFFERTOOSMALL;
    }

    ::CoTaskMemFree(lpstrName);
  }

  return hRet;
}

static void SetDefaultFolder(IFileDialog* pDialog,
                             const base::FilePath file_path) {
  std::wstring directory =
      IsDirectory(file_path) ? file_path.value() : file_path.DirName().value();

  ATL::CComPtr<IShellItem> folder_item;
  HRESULT hr = SHCreateItemFromParsingName(directory.c_str(), NULL,
                                           IID_PPV_ARGS(&folder_item));
  if (SUCCEEDED(hr))
    pDialog->SetFolder(folder_item);
}

static HRESULT ShowFileDialog(IFileDialog* pDialog,
                              const DialogSettings& settings) {
  atom::UnresponsiveSuppressor suppressor;
  HWND parent_window =
      settings.parent_window
          ? static_cast<atom::NativeWindowViews*>(settings.parent_window)
                ->GetAcceleratedWidget()
          : NULL;

  return pDialog->Show(parent_window);
}

static void ApplySettings(IFileDialog* pDialog,
                          const DialogSettings& settings) {
  std::wstring file_part;

  if (!IsDirectory(settings.default_path))
    file_part = settings.default_path.BaseName().value();

  pDialog->SetFileName(file_part.c_str());

  if (!settings.title.empty())
    pDialog->SetTitle(base::UTF8ToUTF16(settings.title).c_str());

  if (!settings.button_label.empty())
    pDialog->SetOkButtonLabel(base::UTF8ToUTF16(settings.button_label).c_str());

  std::vector<std::wstring> buffer;
  std::vector<COMDLG_FILTERSPEC> filterspec;
  ConvertFilters(settings.filters, &buffer, &filterspec);

  if (!filterspec.empty()) {
    pDialog->SetFileTypes(filterspec.size(), filterspec.data());
  }

  // By default, *.* will be added to the file name if file type is "*.*". In
  // Electron, we disable it to make a better experience.
  //
  // From MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/
  // bb775970(v=vs.85).aspx
  //
  // If SetDefaultExtension is not called, the dialog will not update
  // automatically when user choose a new file type in the file dialog.
  //
  // We set file extension to the first none-wildcard extension to make
  // sure the dialog will update file extension automatically.
  for (size_t i = 0; i < filterspec.size(); ++i) {
    if (std::wstring(filterspec[i].pszSpec) != L"*.*") {
      // SetFileTypeIndex is regarded as one-based index.
      pDialog->SetFileTypeIndex(i + 1);
      pDialog->SetDefaultExtension(filterspec[i].pszSpec);
      break;
    }
  }

  if (settings.default_path.IsAbsolute()) {
    SetDefaultFolder(pDialog, settings.default_path);
  }
}

bool ShowOpenDialog(const DialogSettings& settings,
                    std::vector<base::FilePath>* paths) {
  ATL::CComPtr<IFileOpenDialog> pFileOpen;
  HRESULT hr = pFileOpen.CoCreateInstance(CLSID_FileOpenDialog);

  if (FAILED(hr))
    return false;

  DWORD options = FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST;
  if (settings.properties & FILE_DIALOG_OPEN_DIRECTORY)
    options |= FOS_PICKFOLDERS;
  if (settings.properties & FILE_DIALOG_MULTI_SELECTIONS)
    options |= FOS_ALLOWMULTISELECT;
  if (settings.properties & FILE_DIALOG_SHOW_HIDDEN_FILES)
    options |= FOS_FORCESHOWHIDDEN;
  if (settings.properties & FILE_DIALOG_PROMPT_TO_CREATE)
    options |= FOS_CREATEPROMPT;
  pFileOpen->SetOptions(options);

  ApplySettings(pFileOpen, settings);
  hr = ShowFileDialog(pFileOpen, settings);
  if (FAILED(hr))
    return false;

  ATL::CComPtr<IShellItemArray> items;
  hr = pFileOpen->GetResults(&items);
  if (FAILED(hr))
    return false;

  ATL::CComPtr<IShellItem> item;
  DWORD count = 0;
  hr = items->GetCount(&count);
  if (FAILED(hr))
    return false;

  paths->reserve(count);
  for (DWORD i = 0; i < count; ++i) {
    hr = items->GetItemAt(i, &item);
    if (FAILED(hr))
      return false;

    wchar_t file_name[MAX_PATH];
    hr = GetFileNameFromShellItem(item, SIGDN_FILESYSPATH, file_name, MAX_PATH);

    if (FAILED(hr))
      return false;

    paths->push_back(base::FilePath(file_name));
  }

  return true;
}

void ShowOpenDialog(const DialogSettings& settings,
                    const OpenDialogCallback& callback) {
  RunState run_state;
  if (!CreateDialogThread(&run_state)) {
    callback.Run(false, std::vector<base::FilePath>());
    return;
  }

  run_state.dialog_thread->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&RunOpenDialogInNewThread, run_state, settings, callback));
}

bool ShowSaveDialog(const DialogSettings& settings, base::FilePath* path) {
  ATL::CComPtr<IFileSaveDialog> pFileSave;
  HRESULT hr = pFileSave.CoCreateInstance(CLSID_FileSaveDialog);
  if (FAILED(hr))
    return false;

  pFileSave->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST |
                        FOS_OVERWRITEPROMPT);
  ApplySettings(pFileSave, settings);
  hr = ShowFileDialog(pFileSave, settings);

  if (FAILED(hr))
    return false;

  CComPtr<IShellItem> pItem;
  hr = pFileSave->GetResult(&pItem);
  if (FAILED(hr))
    return false;

  PWSTR result_path = nullptr;
  hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &result_path);
  if (!SUCCEEDED(hr))
    return false;

  *path = base::FilePath(result_path);
  CoTaskMemFree(result_path);

  return true;
}

void ShowSaveDialog(const DialogSettings& settings,
                    const SaveDialogCallback& callback) {
  RunState run_state;
  if (!CreateDialogThread(&run_state)) {
    callback.Run(false, base::FilePath());
    return;
  }

  run_state.dialog_thread->task_runner()->PostTask(
      FROM_HERE,
      base::Bind(&RunSaveDialogInNewThread, run_state, settings, callback));
}

}  // namespace file_dialog
