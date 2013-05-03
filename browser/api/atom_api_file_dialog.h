// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_FILE_DIALOG_H_
#define ATOM_BROWSER_API_ATOM_API_FILE_DIALOG_H_

#include "browser/api/atom_api_event_emitter.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace atom {

namespace api {

class FileDialog : public EventEmitter,
                   public ui::SelectFileDialog::Listener {
 public:
  virtual ~FileDialog();

  static void Initialize(v8::Handle<v8::Object> target);

  // ui::SelectFileDialog::Listener implementations:
  virtual void FileSelected(const base::FilePath& path,
                            int index, void* params) OVERRIDE;
  virtual void MultiFilesSelected(
      const std::vector<base::FilePath>& files, void* params) OVERRIDE;
  virtual void FileSelectionCanceled(void* params) OVERRIDE;

 private:
  explicit FileDialog(v8::Handle<v8::Object> wrapper);

  static void FillTypeInfo(ui::SelectFileDialog::FileTypeInfo* file_types,
                           v8::Handle<v8::Array> v8_file_types);

  static v8::Handle<v8::Value> New(const v8::Arguments &args);
  static v8::Handle<v8::Value> SelectFile(const v8::Arguments &args);

  scoped_refptr<ui::SelectFileDialog> dialog_;

  DISALLOW_COPY_AND_ASSIGN(FileDialog);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_FILE_DIALOG_H_
