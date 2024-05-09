// Copyright (c) 2024 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_SELECT_FILE_POLICY_H_
#define SHELL_BROWSER_UI_SELECT_FILE_POLICY_H_

#include "base/memory/raw_ptr.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace content {
class WebContents;
}

class ElectronSelectFilePolicy : public ui::SelectFilePolicy {
 public:
  explicit ElectronSelectFilePolicy(content::WebContents* web_contents);

  ElectronSelectFilePolicy(const ElectronSelectFilePolicy&) = delete;
  ElectronSelectFilePolicy& operator=(const ElectronSelectFilePolicy&) = delete;

  ~ElectronSelectFilePolicy() override;

  // Overridden from ui::SelectFilePolicy:
  bool CanOpenSelectFileDialog() override;
  void SelectFileDenied() override;

 private:
  raw_ptr<content::WebContents, AcrossTasksDanglingUntriaged> web_contents_;
};

#endif  // SHELL_BROWSER_UI_SELECT_FILE_POLICY_H_
