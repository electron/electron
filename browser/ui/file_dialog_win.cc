// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/file_dialog.h"

#include <atlbase.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include "base/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/string_util.h"
#include "base/strings/string_split.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "browser/native_window.h"
#include "third_party/wtl/include/atlapp.h"
#include "third_party/wtl/include/atldlgs.h"

namespace file_dialog {

namespace {

// Distinguish directories from regular files.
bool IsDirectory(const base::FilePath& path) {
  base::PlatformFileInfo file_info;
  return file_util::GetFileInfo(path, &file_info) ?
      file_info.is_directory : path.EndsWithSeparator();
}

// Get the file type description from the registry. This will be "Text Document"
// for .txt files, "JPEG Image" for .jpg files, etc. If the registry doesn't
// have an entry for the file type, we return false, true if the description was
// found. 'file_ext' must be in form ".txt".
static bool GetRegistryDescriptionFromExtension(const std::wstring& file_ext,
                                                std::wstring* reg_description) {
  DCHECK(reg_description);
  base::win::RegKey reg_ext(HKEY_CLASSES_ROOT, file_ext.c_str(), KEY_READ);
  std::wstring reg_app;
  if (reg_ext.ReadValue(NULL, &reg_app) == ERROR_SUCCESS && !reg_app.empty()) {
    base::win::RegKey reg_link(HKEY_CLASSES_ROOT, reg_app.c_str(), KEY_READ);
    if (reg_link.ReadValue(NULL, reg_description) == ERROR_SUCCESS)
      return true;
  }
  return false;
}

// Set up a filter for a Save/Open dialog, which will consist of |file_ext| file
// extensions (internally separated by semicolons), |ext_desc| as the text
// descriptions of the |file_ext| types (optional), and (optionally) the default
// 'All Files' view. The purpose of the filter is to show only files of a
// particular type in a Windows Save/Open dialog box. The resulting filter is
// returned. The filters created here are:
//   1. only files that have 'file_ext' as their extension
//   2. all files (only added if 'include_all_files' is true)
// Example:
//   file_ext: { "*.txt", "*.htm;*.html" }
//   ext_desc: { "Text Document" }
//   returned: "Text Document\0*.txt\0HTML Document\0*.htm;*.html\0"
//             "All Files\0*.*\0\0" (in one big string)
// If a description is not provided for a file extension, it will be retrieved
// from the registry. If the file extension does not exist in the registry, it
// will be omitted from the filter, as it is likely a bogus extension.
void FormatFilterForExtensions(
    std::vector<std::wstring>* file_ext,
    std::vector<std::wstring>* ext_desc,
    bool include_all_files,
    std::vector<COMDLG_FILTERSPEC>* file_types) {
  DCHECK(file_ext->size() >= ext_desc->size());

  if (file_ext->empty())
    include_all_files = true;

  for (size_t i = 0; i < file_ext->size(); ++i) {
    std::wstring ext = (*file_ext)[i];
    std::wstring desc;
    if (i < ext_desc->size())
      desc = (*ext_desc)[i];

    if (ext.empty()) {
      // Force something reasonable to appear in the dialog box if there is no
      // extension provided.
      include_all_files = true;
      continue;
    }

    if (desc.empty()) {
      DCHECK(ext.find(L'.') != std::wstring::npos);
      std::wstring first_extension = ext.substr(ext.find(L'.'));
      size_t first_separator_index = first_extension.find(L';');
      if (first_separator_index != std::wstring::npos)
        first_extension = first_extension.substr(0, first_separator_index);

      // Find the extension name without the preceeding '.' character.
      std::wstring ext_name = first_extension;
      size_t ext_index = ext_name.find_first_not_of(L'.');
      if (ext_index != std::wstring::npos)
        ext_name = ext_name.substr(ext_index);

      if (!GetRegistryDescriptionFromExtension(first_extension, &desc)) {
        // The extension doesn't exist in the registry. Create a description
        // based on the unknown extension type (i.e. if the extension is .qqq,
        // the we create a description "QQQ File (.qqq)").
        include_all_files = true;
        // TODO(zcbenz): should be localized.
        desc = base::i18n::ToUpper(WideToUTF16(ext_name)) + L" File";
      }
      desc += L" (*." + ext_name + L")";

      // Store the description.
      ext_desc->push_back(desc);
    }

    COMDLG_FILTERSPEC spec = { (*ext_desc)[i].c_str(), (*file_ext)[i].c_str() };
    file_types->push_back(spec);
  }

  if (include_all_files) {
    // TODO(zcbenz): Should be localized.
    ext_desc->push_back(L"All Files (*.*)");
    file_ext->push_back(L"*.*");

    COMDLG_FILTERSPEC spec = {
      (*ext_desc)[ext_desc->size() - 1].c_str(),
      (*file_ext)[file_ext->size() - 1].c_str(),
    };
    file_types->push_back(spec);
  }
}

// Generic class to delegate common open/save dialog's behaviours, users need to
// call interface methods via GetPtr().
template <typename T>
class FileDialog {
 public:
  FileDialog(const base::FilePath& default_path,
             const std::string title,
             const std::vector<std::wstring>& file_ext,
             const std::vector<std::wstring>& desc_ext)
      : file_ext_(file_ext),
        desc_ext_(desc_ext) {
    std::vector<COMDLG_FILTERSPEC> filters;
    FormatFilterForExtensions(&file_ext_, &desc_ext_, true, &filters);

    std::wstring file_part;
    if (!IsDirectory(default_path))
      file_part = default_path.BaseName().value();

    dialog_.reset(new T(
        file_part.c_str(),
        FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT,
        NULL,
        filters.data(),
        filters.size()));

    if (!title.empty())
      GetPtr()->SetTitle(UTF8ToUTF16(title).c_str());

    SetDefaultFolder(default_path);
  }

  bool Show(atom::NativeWindow* window) {
    return dialog_->DoModal(window->GetNativeWindow()) == IDOK;
  }

  T* GetDialog() { return dialog_.get(); }

  IFileDialog* GetPtr() const { return dialog_->GetPtr(); }

  const std::vector<std::wstring> file_ext() const { return file_ext_; }

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
      GetPtr()->SetDefaultFolder(folder_item);
  }

  scoped_ptr<T> dialog_;

  std::vector<std::wstring> file_ext_;
  std::vector<std::wstring> desc_ext_;
  std::vector<COMDLG_FILTERSPEC> filters_;

  DISALLOW_COPY_AND_ASSIGN(FileDialog);
};

}  // namespace

bool ShowOpenDialog(const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  return false;
}

bool ShowSaveDialog(atom::NativeWindow* window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path) {
  // TODO(zcbenz): Accept custom filters from caller.
  std::vector<std::wstring> file_ext;
  std::wstring extension = default_path.Extension();
  if (!extension.empty())
    file_ext.push_back(extension.insert(0, L"*"));

  FileDialog<CShellFileSaveDialog> save_dialog(
      default_path,
      title,
      file_ext,
      std::vector<std::wstring>());
  if (!save_dialog.Show(window))
    return false;

  wchar_t file_name[MAX_PATH];
  HRESULT hr = save_dialog.GetDialog()->GetFilePath(file_name, MAX_PATH);
  if (FAILED(hr))
    return false;

  // Append extension according to selected filter.
  UINT filter_index = 1;
  save_dialog.GetPtr()->GetFileTypeIndex(&filter_index);
  std::wstring selected_filter = save_dialog.file_ext()[filter_index - 1];
  if (selected_filter != L"*.*") {
    std::wstring result = file_name;
    if (!EndsWith(result, selected_filter.substr(1), false)) {
      if (result[result.length() - 1] != L'.')
        result.push_back(L'.');
      result.append(selected_filter.substr(2));
      *path = base::FilePath(result);
      return true;
    }
  }

  *path = base::FilePath(file_name);
  return true;
}

}  // namespace file_dialog
