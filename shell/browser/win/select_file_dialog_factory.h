// Copyright 2024 Microsoft GmbH.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_WIN_SELECT_FILE_DIALOG_FACTORY_H_
#define ELECTRON_SHELL_BROWSER_WIN_SELECT_FILE_DIALOG_FACTORY_H_

#include <memory>

#include "ui/shell_dialogs/select_file_dialog_factory.h"

class ElectronSelectFileDialogFactory : public ui::SelectFileDialogFactory {
 public:
  ElectronSelectFileDialogFactory();

  ElectronSelectFileDialogFactory(const ElectronSelectFileDialogFactory&) =
      delete;
  ElectronSelectFileDialogFactory& operator=(
      const ElectronSelectFileDialogFactory&) = delete;

  ~ElectronSelectFileDialogFactory() override;

  // ui::SelectFileDialogFactory:
  ui::SelectFileDialog* Create(
      ui::SelectFileDialog::Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy) override;
};

#endif  // ELECTRON_SHELL_BROWSER_WIN_SELECT_FILE_DIALOG_FACTORY_H_
