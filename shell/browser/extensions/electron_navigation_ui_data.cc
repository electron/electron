// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/electron_navigation_ui_data.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"
#include "extensions/common/constants.h"

namespace extensions {

ElectronNavigationUIData::ElectronNavigationUIData() = default;

ElectronNavigationUIData::ElectronNavigationUIData(
    content::NavigationHandle* navigation_handle) {
  extension_data_ = std::make_unique<ExtensionNavigationUIData>(
      navigation_handle, extension_misc::kUnknownTabId,
      extension_misc::kUnknownWindowId);
}

ElectronNavigationUIData::~ElectronNavigationUIData() = default;

std::unique_ptr<content::NavigationUIData> ElectronNavigationUIData::Clone() {
  std::unique_ptr<ElectronNavigationUIData> copy =
      std::make_unique<ElectronNavigationUIData>();

  if (extension_data_)
    copy->SetExtensionNavigationUIData(extension_data_->DeepCopy());

  return std::move(copy);
}

void ElectronNavigationUIData::SetExtensionNavigationUIData(
    std::unique_ptr<ExtensionNavigationUIData> extension_data) {
  extension_data_ = std::move(extension_data);
}

}  // namespace extensions
