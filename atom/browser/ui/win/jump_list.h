// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_WIN_JUMP_LIST_H_
#define ATOM_BROWSER_UI_WIN_JUMP_LIST_H_

#include <atlbase.h>
#include <shobjidl.h>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"

namespace atom {

enum class JumpListResult : int {
  SUCCESS = 0,
  // In JS code this error will manifest as an exception.
  ARGUMENT_ERROR = 1,
  // Generic error, the runtime logs may provide some clues.
  GENERIC_ERROR = 2,
  // Custom categories can't contain separators.
  CUSTOM_CATEGORY_SEPARATOR_ERROR = 3,
  // The app isn't registered to handle a file type found in a custom category.
  MISSING_FILE_TYPE_REGISTRATION_ERROR = 4,
  // Custom categories can't be created due to user privacy settings.
  CUSTOM_CATEGORY_ACCESS_DENIED_ERROR = 5,
};

struct JumpListItem {
  enum class Type {
    // A task will launch an app (usually the one that created the Jump List)
    // with specific arguments.
    TASK,
    // Separators can only be inserted between items in the standard Tasks
    // category, they can't appear in custom categories.
    SEPARATOR,
    // A file link will open a file using the app that created the Jump List,
    // for this to work the app must be registered as a handler for the file
    // type (though the app doesn't have to be the default handler).
    FILE
  };

  Type type = Type::TASK;
  // For tasks this is the path to the program executable, for file links this
  // is the full filename.
  base::FilePath path;
  base::string16 arguments;
  base::string16 title;
  base::string16 description;
  base::FilePath icon_path;
  int icon_index = 0;
};

struct JumpListCategory {
  enum class Type {
    // A custom category can contain tasks and files, but not separators.
    CUSTOM,
    // Frequent/Recent categories are managed by the OS, their name and items
    // can't be set by the app (though items can be set indirectly).
    FREQUENT,
    RECENT,
    // The standard Tasks category can't be renamed by the app, but the app
    // can set the items that should appear in this category, and those items
    // can include tasks, files, and separators.
    TASKS
  };

  Type type = Type::TASKS;
  base::string16 name;
  std::vector<JumpListItem> items;
};

// Creates or removes a custom Jump List for an app.
// See https://msdn.microsoft.com/en-us/library/windows/desktop/gg281362.aspx
class JumpList {
 public:
  // |app_id| must be the Application User Model ID of the app for which the
  // custom Jump List should be created/removed, it's usually obtained by
  // calling GetCurrentProcessExplicitAppUserModelID().
  explicit JumpList(const base::string16& app_id);

  // Starts a new transaction, must be called before appending any categories,
  // aborting or committing. After the method returns |min_items| will indicate
  // the minimum number of items that will be displayed in the Jump List, and
  // |removed_items| (if not null) will contain all the items the user has
  // unpinned from the Jump List. Both parameters are optional.
  bool Begin(int* min_items = nullptr,
             std::vector<JumpListItem>* removed_items = nullptr);
  // Abandons any changes queued up since Begin() was called.
  bool Abort();
  // Commits any changes queued up since Begin() was called.
  bool Commit();
  // Deletes the custom Jump List and restores the default Jump List.
  bool Delete();
  // Appends a category to the custom Jump List.
  JumpListResult AppendCategory(const JumpListCategory& category);
  // Appends categories to the custom Jump List.
  JumpListResult AppendCategories(
    const std::vector<JumpListCategory>& categories);

 private:
  base::string16 app_id_;
  CComPtr<ICustomDestinationList> destinations_;

  DISALLOW_COPY_AND_ASSIGN(JumpList);
};

}  // namespace atom

#endif  // ATOM_BROWSER_UI_WIN_JUMP_LIST_H_
