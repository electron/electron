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
  FILE_DIALOG_OPEN_FILE          = 1 << 0,
  FILE_DIALOG_OPEN_DIRECTORY     = 1 << 1,
  FILE_DIALOG_MULTI_SELECTIONS   = 1 << 2,
  FILE_DIALOG_CREATE_DIRECTORY   = 1 << 3,
  FILE_DIALOG_SHOW_HIDDEN_FILES  = 1 << 4,
  FILE_DIALOG_PROMPT_TO_CREATE   = 1 << 5,
  FILE_DIALOG_NO_RESOLVE_ALIASES = 1 << 6,
  FILE_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY = 1 << 7,
};

typedef base::Callback<void(
    bool result, const std::vector<base::FilePath>& paths)> OpenDialogCallback;

typedef base::Callback<void(
    bool result, const base::FilePath& path)> SaveDialogCallback;

struct DialogSettings {
  atom::NativeWindow* parent_window = nullptr;
  std::string title;
  std::string message;
  std::string button_label;
  std::string name_field_label;
  base::FilePath default_path;
  Filters filters;
  int properties = 0;
  bool shows_tag_field = true;
  bool force_detached = false;
};

bool ShowOpenDialog(const DialogSettings& settings,
                    std::vector<base::FilePath>* paths);

void ShowOpenDialog(const DialogSettings& settings,
                    const OpenDialogCallback& callback);

bool ShowSaveDialog(const DialogSettings& settings,
                    base::FilePath* path);

void ShowSaveDialog(const DialogSettings& settings,
                    const SaveDialogCallback& callback);

}  // namespace file_dialog

#endif  // ATOM_BROWSER_UI_FILE_DIALOG_H_
