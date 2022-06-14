// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_

#include <memory>

#include "content/public/browser/navigation_ui_data.h"
#include "extensions/browser/extension_navigation_ui_data.h"

namespace extensions {

// PlzNavigate
// Contains data that is passed from the UI thread to the IO thread at the
// beginning of each navigation. The class is instantiated on the UI thread,
// then a copy created using Clone is passed to the content::ResourceRequestInfo
// on the IO thread.
class ElectronNavigationUIData : public content::NavigationUIData {
 public:
  ElectronNavigationUIData();
  explicit ElectronNavigationUIData(
      content::NavigationHandle* navigation_handle);
  ~ElectronNavigationUIData() override;

  // disable copy
  ElectronNavigationUIData(const ElectronNavigationUIData&) = delete;
  ElectronNavigationUIData& operator=(const ElectronNavigationUIData&) = delete;

  // Creates a new ChromeNavigationUIData that is a deep copy of the original.
  // Any changes to the original after the clone is created will not be
  // reflected in the clone.  |extension_data_| is deep copied.
  std::unique_ptr<content::NavigationUIData> Clone() override;

  void SetExtensionNavigationUIData(
      std::unique_ptr<ExtensionNavigationUIData> extension_data);

  ExtensionNavigationUIData* GetExtensionNavigationUIData() const {
    return extension_data_.get();
  }

 private:
  // Manages the lifetime of optional ExtensionNavigationUIData information.
  std::unique_ptr<ExtensionNavigationUIData> extension_data_;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_ELECTRON_NAVIGATION_UI_DATA_H_
