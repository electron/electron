// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/file_dialog.h"

#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include "base/file_util.h"
#include "base/string_util.h"
#include "base/win/windows_version.h"
#include "browser/native_window.h"

namespace file_dialog {

namespace {

// Distinguish directories from regular files.
bool IsDirectory(const base::FilePath& path) {
  base::PlatformFileInfo file_info;
  return file_util::GetFileInfo(path, &file_info) ?
      file_info.is_directory : path.EndsWithSeparator();
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

  // TODO(zcbenz): Should support filters.
  save_as.lpstrFilter = NULL;
  save_as.nFilterIndex = 0;

  save_as.lpstrCustomFilter = NULL;
  save_as.nMaxCustFilter = 0;
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
  *path = base::FilePath(save_as.lpstrFile);
  return true;
}

}  // namespace file_dialog
