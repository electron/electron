// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/file_dialog.h"

#include <atlbase.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include "atom/browser/native_window_views.h"
#include "atom/browser/unresponsive_suppressor.h"
#include "base/files/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/win/registry.h"
#include "third_party/wtl/include/atlapp.h"
#include "third_party/wtl/include/atldlgs.h"

namespace file_dialog {

namespace {

// Distinguish directories from regular files.
bool IsDirectory(const base::FilePath& path) {
  base::File::Info file_info;
  return base::GetFileInfo(path, &file_info) ?
      file_info.is_directory : path.EndsWithSeparator();
}

void ConvertFilters(const Filters& filters,
                    std::vector<std::wstring>* buffer,
                    std::vector<COMDLG_FILTERSPEC>* filterspec) {
  if (filters.empty()) {
    COMDLG_FILTERSPEC spec = { L"All Files (*.*)", L"*.*" };
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

// Generic class to delegate common open/save dialog's behaviours, users need to
// call interface methods via GetPtr().
template <typename T>
class FileDialog {
 public:
  FileDialog(const base::FilePath& default_path,
             const std::string& title,
             const std::string& button_label,
             const Filters& filters, int options) {
    std::wstring file_part;
    if (!IsDirectory(default_path))
      file_part = default_path.BaseName().value();

    std::vector<std::wstring> buffer;
    std::vector<COMDLG_FILTERSPEC> filterspec;
    ConvertFilters(filters, &buffer, &filterspec);

    dialog_.reset(new T(file_part.c_str(), options, NULL,
                        filterspec.data(), filterspec.size()));

    if (!title.empty())
      GetPtr()->SetTitle(base::UTF8ToUTF16(title).c_str());

    if (!button_label.empty())
      GetPtr()->SetOkButtonLabel(base::UTF8ToUTF16(button_label).c_str());

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
        GetPtr()->SetFileTypeIndex(i+1);
        GetPtr()->SetDefaultExtension(filterspec[i].pszSpec);
        break;
      }
    }

    SetDefaultFolder(default_path);
  }

  bool Show(atom::NativeWindow* parent_window) {
    atom::UnresponsiveSuppressor suppressor;
    HWND window = parent_window ? static_cast<atom::NativeWindowViews*>(
        parent_window)->GetAcceleratedWidget() :
        NULL;
    return dialog_->DoModal(window) == IDOK;
  }

  T* GetDialog() { return dialog_.get(); }

  IFileDialog* GetPtr() const { return dialog_->GetPtr(); }

 private:
  // Set up the initial directory for the dialog.
  void SetDefaultFolder(const base::FilePath file_path) {
    std::wstring directory = IsDirectory(file_path) ?
        file_path.value() :
        file_path.DirName().value();

    ATL::CComPtr<IShellItem> folder_item;
    HRESULT hr = SHCreateItemFromParsingName(directory.c_str(),
                                             NULL,
                                             IID_PPV_ARGS(&folder_item));
    if (SUCCEEDED(hr))
      GetPtr()->SetFolder(folder_item);
  }

  std::unique_ptr<T> dialog_;

  DISALLOW_COPY_AND_ASSIGN(FileDialog);
};

struct RunState {
  base::Thread* dialog_thread;
  base::MessageLoop* ui_message_loop;
};

bool CreateDialogThread(RunState* run_state) {
  std::unique_ptr<base::Thread> thread(
      new base::Thread(ATOM_PRODUCT_NAME "FileDialogThread"));
  thread->init_com_with_mta(false);
  if (!thread->Start())
    return false;

  run_state->dialog_thread = thread.release();
  run_state->ui_message_loop = base::MessageLoop::current();
  return true;
}

void RunOpenDialogInNewThread(const RunState& run_state,
                              atom::NativeWindow* parent,
                              const std::string& title,
                              const std::string& button_label,
                              const base::FilePath& default_path,
                              const Filters& filters,
                              int properties,
                              const OpenDialogCallback& callback) {
  std::vector<base::FilePath> paths;
  bool result = ShowOpenDialog(parent, title, button_label, default_path,
                               filters, properties, &paths);
  run_state.ui_message_loop->PostTask(FROM_HERE,
                                      base::Bind(callback, result, paths));
  run_state.ui_message_loop->DeleteSoon(FROM_HERE, run_state.dialog_thread);
}

void RunSaveDialogInNewThread(const RunState& run_state,
                              atom::NativeWindow* parent,
                              const std::string& title,
                              const std::string& button_label,
                              const base::FilePath& default_path,
                              const Filters& filters,
                              const SaveDialogCallback& callback) {
  base::FilePath path;
  bool result = ShowSaveDialog(parent, title, button_label, default_path,
                               filters, &path);
  run_state.ui_message_loop->PostTask(FROM_HERE,
                                      base::Bind(callback, result, path));
  run_state.ui_message_loop->DeleteSoon(FROM_HERE, run_state.dialog_thread);
}

}  // namespace

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  int options = FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST;
  if (properties & FILE_DIALOG_OPEN_DIRECTORY)
    options |= FOS_PICKFOLDERS;
  if (properties & FILE_DIALOG_MULTI_SELECTIONS)
    options |= FOS_ALLOWMULTISELECT;
  if (properties & FILE_DIALOG_SHOW_HIDDEN_FILES)
    options |= FOS_FORCESHOWHIDDEN;

  FileDialog<CShellFileOpenDialog> open_dialog(
      default_path, title, button_label, filters, options);
  if (!open_dialog.Show(parent_window))
    return false;

  ATL::CComPtr<IShellItemArray> items;
  HRESULT hr = static_cast<IFileOpenDialog*>(open_dialog.GetPtr())->GetResults(
      &items);
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
    hr = CShellFileOpenDialog::GetFileNameFromShellItem(
        item, SIGDN_FILESYSPATH, file_name, MAX_PATH);
    if (FAILED(hr))
      return false;

    paths->push_back(base::FilePath(file_name));
  }

  return true;
}

void ShowOpenDialog(atom::NativeWindow* parent,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    const OpenDialogCallback& callback) {
  RunState run_state;
  if (!CreateDialogThread(&run_state)) {
    callback.Run(false, std::vector<base::FilePath>());
    return;
  }

  run_state.dialog_thread->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&RunOpenDialogInNewThread, run_state, parent, title,
                 button_label, default_path, filters, properties, callback));
}

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    base::FilePath* path) {
  FileDialog<CShellFileSaveDialog> save_dialog(
      default_path, title, button_label, filters,
      FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT);
  if (!save_dialog.Show(parent_window))
    return false;

  wchar_t buffer[MAX_PATH];
  HRESULT hr = save_dialog.GetDialog()->GetFilePath(buffer, MAX_PATH);
  if (FAILED(hr))
    return false;

  *path = base::FilePath(buffer);
  return true;
}

void ShowSaveDialog(atom::NativeWindow* parent,
                    const std::string& title,
                    const std::string& button_label,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    const SaveDialogCallback& callback) {
  RunState run_state;
  if (!CreateDialogThread(&run_state)) {
    callback.Run(false, base::FilePath());
    return;
  }

  run_state.dialog_thread->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&RunSaveDialogInNewThread, run_state, parent, title,
                 button_label, default_path, filters, callback));
}

}  // namespace file_dialog
