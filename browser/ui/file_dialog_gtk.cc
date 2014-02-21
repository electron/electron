// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/ui/file_dialog.h"

#include "base/callback.h"

namespace file_dialog {

bool ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths) {
  return false;
}

void ShowOpenDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    const OpenDialogCallback& callback) {
  callback.Run(false, std::vector<base::FilePath>());
}

bool ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path) {
  return false;
}

void ShowSaveDialog(atom::NativeWindow* parent_window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    const SaveDialogCallback& callback) {
  callback.Run(false, base::FilePath());
}

}  // namespace file_dialog
