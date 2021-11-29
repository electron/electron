// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/font_access_chooser.h"
#include "content/public/browser/font_access_delegate.h"

class ElectronFontAccessDelegate : public content::FontAccessDelegate {
 public:
  ElectronFontAccessDelegate();
  ~ElectronFontAccessDelegate() override;

  // disable copy
  ElectronFontAccessDelegate(const ElectronFontAccessDelegate&) = delete;
  ElectronFontAccessDelegate& operator=(const ElectronFontAccessDelegate&) =
      delete;

  std::unique_ptr<content::FontAccessChooser> RunChooser(
      content::RenderFrameHost* frame,
      const std::vector<std::string>& selection,
      content::FontAccessChooser::Callback callback) override;
};

#endif  // ELECTRON_SHELL_BROWSER_FONT_ELECTRON_FONT_ACCESS_DELEGATE_H_
