// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_FILE_DIALOG_H_
#define ELECTRON_SHELL_BROWSER_UI_FILE_DIALOG_H_

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"

namespace electron {
class NativeWindow;
}

namespace file_dialog {

// <description, extensions>
typedef std::pair<std::string, std::vector<std::string>> Filter;
typedef std::vector<Filter> Filters;

enum OpenFileDialogProperty {
  OPEN_DIALOG_OPEN_FILE = 1 << 0,
  OPEN_DIALOG_OPEN_DIRECTORY = 1 << 1,
  OPEN_DIALOG_MULTI_SELECTIONS = 1 << 2,
  OPEN_DIALOG_CREATE_DIRECTORY = 1 << 3,  // macOS
  OPEN_DIALOG_SHOW_HIDDEN_FILES = 1 << 4,
  OPEN_DIALOG_PROMPT_TO_CREATE = 1 << 5,                // Windows
  OPEN_DIALOG_NO_RESOLVE_ALIASES = 1 << 6,              // macOS
  OPEN_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY = 1 << 7,  // macOS
  FILE_DIALOG_DONT_ADD_TO_RECENT = 1 << 8,              // Windows
};

enum SaveFileDialogProperty {
  SAVE_DIALOG_CREATE_DIRECTORY = 1 << 0,
  SAVE_DIALOG_SHOW_HIDDEN_FILES = 1 << 1,
  SAVE_DIALOG_TREAT_PACKAGE_APP_AS_DIRECTORY = 1 << 2,  // macOS
  SAVE_DIALOG_SHOW_OVERWRITE_CONFIRMATION = 1 << 3,     // Linux
  SAVE_DIALOG_DONT_ADD_TO_RECENT = 1 << 4,              // Windows
};

struct DialogSettings {
  electron::NativeWindow* parent_window = nullptr;
  std::string title;
  std::string message;
  std::string button_label;
  std::string name_field_label;
  base::FilePath default_path;
  Filters filters;
  int properties = 0;
  bool shows_tag_field = true;
  bool force_detached = false;
  bool security_scoped_bookmarks = false;

  DialogSettings();
  DialogSettings(const DialogSettings&);
  ~DialogSettings();
};

bool ShowOpenDialogSync(const DialogSettings& settings,
                        std::vector<base::FilePath>* paths);

void ShowOpenDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise);

bool ShowSaveDialogSync(const DialogSettings& settings, base::FilePath* path);

void ShowSaveDialog(const DialogSettings& settings,
                    gin_helper::Promise<gin_helper::Dictionary> promise);

}  // namespace file_dialog

#endif  // ELECTRON_SHELL_BROWSER_UI_FILE_DIALOG_H_
