// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_NAVIGATION_UI_DATA_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_NAVIGATION_UI_DATA_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_ui_data.h"
#include "extensions/browser/extension_navigation_ui_data.h"

namespace extensions {

// PlzNavigate
// Contains data that is passed from the UI thread to the IO thread at the
// beginning of each navigation. The class is instantiated on the UI thread,
// then a copy created using Clone is passed to the content::ResourceRequestInfo
// on the IO thread.
class ShellNavigationUIData : public content::NavigationUIData {
 public:
  ShellNavigationUIData();
  explicit ShellNavigationUIData(content::NavigationHandle* navigation_handle);
  ~ShellNavigationUIData() override;

  // Creates a new ChromeNavigationUIData that is a deep copy of the original.
  // Any changes to the original after the clone is created will not be
  // reflected in the clone.  |extension_data_| is deep copied.
  std::unique_ptr<content::NavigationUIData> Clone() const override;

  void SetExtensionNavigationUIData(
      std::unique_ptr<ExtensionNavigationUIData> extension_data);

  ExtensionNavigationUIData* GetExtensionNavigationUIData() const {
    return extension_data_.get();
  }

 private:
  // Manages the lifetime of optional ExtensionNavigationUIData information.
  std::unique_ptr<ExtensionNavigationUIData> extension_data_;

  DISALLOW_COPY_AND_ASSIGN(ShellNavigationUIData);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_NAVIGATION_UI_DATA_H_
