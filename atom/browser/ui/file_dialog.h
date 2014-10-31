// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_FILE_DIALOG_H_
#define ATOM_BROWSER_UI_FILE_DIALOG_H_

#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"

namespace atom {
class NativeWindow;
}

namespace file_dialog {

// <description, extensions>
typedef std::pair<std::string, std::vector<std::string> > Filter;
typedef std::vector<Filter> Filters;

enum FileDialogProperty {
  FILE_DIALOG_OPEN_FILE        = 1 << 0,
  FILE_DIALOG_OPEN_DIRECTORY   = 1 << 1,
  FILE_DIALOG_MULTI_SELECTIONS = 1 << 2,
  FILE_DIALOG_CREATE_DIRECTORY = 1 << 3,
};

typedef base::Callback<void(
    bool result, const std::vector<base::FilePath>& paths)> OpenDialogCallback;

typedef base::Callback<void(
    bool result, const base::FilePath& path)> SaveDialogCallback;

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    std::vector<base::FilePath>* paths);

void ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    int properties,
                    const OpenDialogCallback& callback);

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    base::FilePath* path);

void ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const Filters& filters,
                    const SaveDialogCallback& callback);

}  // namespace file_dialog

#endif  // ATOM_BROWSER_UI_FILE_DIALOG_H_
