// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/file_dialog.h"

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include "base/file_util.h"
#include "base/i18n/case_conversion.h"
#include "base/string_util.h"
#include "base/strings/string_split.h"
#include "base/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "browser/native_window.h"

namespace file_dialog {

namespace {

// Given |extension|, if it's not empty, then remove the leading dot.
std::wstring GetExtensionWithoutLeadingDot(const std::wstring& extension) {
  DCHECK(extension.empty() || extension[0] == L'.');
  return extension.empty() ? extension : extension.substr(1);
}

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
std::wstring FormatFilterForExtensions(
    const std::vector<std::wstring>& file_ext,
    const std::vector<std::wstring>& ext_desc,
    bool include_all_files) {
  const std::wstring all_ext = L"*.*";
  // TODO(zcbenz): Should be localized.
  const std::wstring all_desc = L"All Files";

  DCHECK(file_ext.size() >= ext_desc.size());

  if (file_ext.empty())
    include_all_files = true;

  std::wstring result;

  for (size_t i = 0; i < file_ext.size(); ++i) {
    std::wstring ext = file_ext[i];
    std::wstring desc;
    if (i < ext_desc.size())
      desc = ext_desc[i];

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
        desc = base::i18n::ToUpper(WideToUTF16(ext_name)) + L" File (."
                                                          + ext_name
                                                          + L")";
      }
      if (desc.empty())
        desc = L"*." + ext_name;
    }

    result.append(desc.c_str(), desc.size() + 1);  // Append NULL too.
    result.append(ext.c_str(), ext.size() + 1);
  }

  if (include_all_files) {
    result.append(all_desc.c_str(), all_desc.size() + 1);
    result.append(all_ext.c_str(), all_ext.size() + 1);
  }

  result.append(1, '\0');  // Double NULL required.
  return result;
}

// This function takes the output of a SaveAs dialog: a filename, a filter and
// the extension originally suggested to the user (shown in the dialog box) and
// returns back the filename with the appropriate extension tacked on. If the
// user requests an unknown extension and is not using the 'All files' filter,
// the suggested extension will be appended, otherwise we will leave the
// filename unmodified. |filename| should contain the filename selected in the
// SaveAs dialog box and may include the path, |filter_selected| should be
// '*.something', for example '*.*' or it can be blank (which is treated as
// *.*). |suggested_ext| should contain the extension without the dot (.) in
// front, for example 'jpg'.
std::wstring AppendExtensionIfNeeded(
    const std::wstring& filename,
    const std::wstring& filter_selected,
    const std::wstring& suggested_ext) {
  DCHECK(!filename.empty());
  std::wstring return_value = filename;

  // If we wanted a specific extension, but the user's filename deleted it or
  // changed it to something that the system doesn't understand, re-append.
  // Careful: Checking net::GetMimeTypeFromExtension() will only find
  // extensions with a known MIME type, which many "known" extensions on Windows
  // don't have.  So we check directly for the "known extension" registry key.
  std::wstring file_extension(
      GetExtensionWithoutLeadingDot(base::FilePath(filename).Extension()));
  std::wstring key(L"." + file_extension);
  if (!(filter_selected.empty() || filter_selected == L"*.*") &&
      !base::win::RegKey(HKEY_CLASSES_ROOT, key.c_str(), KEY_READ).Valid() &&
      file_extension != suggested_ext) {
    if (return_value[return_value.length() - 1] != L'.')
      return_value.append(L".");
    return_value.append(suggested_ext);
  }

  // Strip any trailing dots, which Windows doesn't allow.
  size_t index = return_value.find_last_not_of(L'.');
  if (index < return_value.size() - 1)
    return_value.resize(index + 1);

  return return_value;
}

// Enforce visible dialog box.
UINT_PTR CALLBACK SaveAsDialogHook(HWND dialog, UINT message,
                                   WPARAM wparam, LPARAM lparam) {
  static const UINT kPrivateMessage = 0x2F3F;
  switch (message) {
    case WM_INITDIALOG: {
      // Do nothing here. Just post a message to defer actual processing.
      PostMessage(dialog, kPrivateMessage, 0, 0);
      return TRUE;
    }
    case kPrivateMessage: {
      // The dialog box is the parent of the current handle.
      HWND real_dialog = GetParent(dialog);

      // Retrieve the final size.
      RECT dialog_rect;
      GetWindowRect(real_dialog, &dialog_rect);

      // Verify that the upper left corner is visible.
      POINT point = { dialog_rect.left, dialog_rect.top };
      HMONITOR monitor1 = MonitorFromPoint(point, MONITOR_DEFAULTTONULL);
      point.x = dialog_rect.right;
      point.y = dialog_rect.bottom;

      // Verify that the lower right corner is visible.
      HMONITOR monitor2 = MonitorFromPoint(point, MONITOR_DEFAULTTONULL);
      if (monitor1 && monitor2)
        return 0;

      // Some part of the dialog box is not visible, fix it by moving is to the
      // client rect position of the browser window.
      HWND parent_window = GetParent(real_dialog);
      if (!parent_window)
        return 0;
      WINDOWINFO parent_info;
      parent_info.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(parent_window, &parent_info);
      SetWindowPos(real_dialog, NULL,
                   parent_info.rcClient.left,
                   parent_info.rcClient.top,
                   0, 0,  // Size.
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE |
                   SWP_NOZORDER);

      return 0;
    }
  }
  return 0;
}

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
  std::wstring file_ext = default_path.Extension().insert(0, L"*");
  std::wstring filter = FormatFilterForExtensions(
      std::vector<std::wstring>(1, file_ext),
      std::vector<std::wstring>(),
      true);

  std::wstring file_part = default_path.BaseName().value();
  // If the default_path is a root directory, file_part will be '\', and the
  // call to GetSaveFileName below will fail.
  if (file_part.size() == 1 && file_part[0] == L'\\')
    file_part.clear();

  // The size of the in/out buffer in number of characters we pass to win32
  // GetSaveFileName.  From MSDN "The buffer must be large enough to store the
  // path and file name string or strings, including the terminating NULL
  // character.  ... The buffer should be at least 256 characters long.".
  // _IsValidPathComDlg does a copy expecting at most MAX_PATH, otherwise will
  // result in an error of FNERR_INVALIDFILENAME.  So we should only pass the
  // API a buffer of at most MAX_PATH.
  wchar_t file_name[MAX_PATH];
  base::wcslcpy(file_name, file_part.c_str(), arraysize(file_name));

  OPENFILENAME save_as;
  // We must do this otherwise the ofn's FlagsEx may be initialized to random
  // junk in release builds which can cause the Places Bar not to show up!
  ZeroMemory(&save_as, sizeof(save_as));
  save_as.lStructSize = sizeof(OPENFILENAME);
  save_as.hwndOwner = window->GetNativeWindow();
  save_as.hInstance = NULL;

  save_as.lpstrFilter = filter.empty() ? NULL : filter.c_str();

  save_as.lpstrCustomFilter = NULL;
  save_as.nMaxCustFilter = 0;
  save_as.nFilterIndex = 1;
  save_as.lpstrFile = file_name;
  save_as.nMaxFile = arraysize(file_name);
  save_as.lpstrFileTitle = NULL;
  save_as.nMaxFileTitle = 0;

  // Set up the initial directory for the dialog.
  std::wstring directory;
  if (IsDirectory(default_path)) {
    directory = default_path.value();
    file_part.clear();
  } else {
    directory = default_path.DirName().value();
  }

  save_as.lpstrInitialDir = directory.c_str();
  save_as.lpstrTitle = NULL;
  save_as.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING |
                  OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
  save_as.lpstrDefExt = NULL;  // default extension, ignored for now.
  save_as.lCustData = NULL;

  if (base::win::GetVersion() < base::win::VERSION_VISTA) {
    // The save as on Windows XP remembers its last position,
    // and if the screen resolution changed, it will be off screen.
    save_as.Flags |= OFN_ENABLEHOOK;
    save_as.lpfnHook = &SaveAsDialogHook;
  }

  // Must be NULL or 0.
  save_as.pvReserved = NULL;
  save_as.dwReserved = 0;

  if (!GetSaveFileName(&save_as)) {
    // Zero means the dialog was closed, otherwise we had an error.
    DWORD error_code = CommDlgExtendedError();
    if (error_code != 0) {
      NOTREACHED() << "GetSaveFileName failed with code: " << error_code;
    }
    return false;
  }

  // Return the user's choice.
  *path = base::FilePath();

  // Figure out what filter got selected from the vector with embedded nulls.
  // NOTE: The filter contains a string with embedded nulls, such as:
  // JPG Image\0*.jpg\0All files\0*.*\0\0
  // The filter index is 1-based index for which pair got selected. So, using
  // the example above, if the first index was selected we need to skip 1
  // instance of null to get to "*.jpg".
  std::vector<std::wstring> filters;
  if (!filter.empty() && save_as.nFilterIndex > 0)
    base::SplitString(filter, '\0', &filters);
  std::wstring filter_selected;
  if (!filters.empty())
    filter_selected = filters[(2 * (save_as.nFilterIndex - 1)) + 1];

  // Get the extension that was suggested to the user (when the Save As dialog
  // was opened).
  std::wstring suggested_ext =
    GetExtensionWithoutLeadingDot(default_path.Extension());

  *path = base::FilePath(AppendExtensionIfNeeded(
        save_as.lpstrFile, filter_selected, suggested_ext));
  return true;
}

}  // namespace file_dialog
