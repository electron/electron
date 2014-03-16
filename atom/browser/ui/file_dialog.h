// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_FILE_DIALOG_H_
#define ATOM_BROWSER_UI_FILE_DIALOG_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"

namespace atom {
class NativeWindow;
}

namespace file_dialog {

enum FileDialogProperty {
  FILE_DIALOG_OPEN_FILE        = 1,
  FILE_DIALOG_OPEN_DIRECTORY   = 2,
  FILE_DIALOG_MULTI_SELECTIONS = 4,
  FILE_DIALOG_CREATE_DIRECTORY = 8,
};

typedef base::Callback<void(
    bool result, const std::vector<base::FilePath>& paths)> OpenDialogCallback;

typedef base::Callback<void(
    bool result, const base::FilePath& path)> SaveDialogCallback;

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths);

void ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    const OpenDialogCallback& callback);

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path);

void ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const SaveDialogCallback& callback);

}  // namespace file_dialog

#endif  // ATOM_BROWSER_UI_FILE_DIALOG_H_
