// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BROWSER_UI_FILE_DIALOG_H_
#define BROWSER_UI_FILE_DIALOG_H_

#include <string>
#include <vector>

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

bool ShowOpenDialog(const std::string& title,
                    const base::FilePath& default_path,
                    int properties,
                    std::vector<base::FilePath>* paths);

bool ShowSaveDialog(atom::NativeWindow* window,
                    const std::string& title,
                    const base::FilePath& default_path,
                    base::FilePath* path);

}  // namespace file_dialog

#endif  // BROWSER_UI_FILE_DIALOG_H_
