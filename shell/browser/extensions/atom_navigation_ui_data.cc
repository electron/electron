// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/atom_navigation_ui_data.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"
#include "extensions/common/constants.h"

namespace extensions {

AtomNavigationUIData::AtomNavigationUIData() {}

AtomNavigationUIData::AtomNavigationUIData(
    content::NavigationHandle* navigation_handle) {
  extension_data_ = std::make_unique<ExtensionNavigationUIData>(
      navigation_handle, extension_misc::kUnknownTabId,
      extension_misc::kUnknownWindowId);
}

AtomNavigationUIData::~AtomNavigationUIData() {}

std::unique_ptr<content::NavigationUIData> AtomNavigationUIData::Clone() {
  std::unique_ptr<AtomNavigationUIData> copy =
      std::make_unique<AtomNavigationUIData>();

  if (extension_data_)
    copy->SetExtensionNavigationUIData(extension_data_->DeepCopy());

  return std::move(copy);
}

void AtomNavigationUIData::SetExtensionNavigationUIData(
    std::unique_ptr<ExtensionNavigationUIData> extension_data) {
  extension_data_ = std::move(extension_data);
}

}  // namespace extensions
