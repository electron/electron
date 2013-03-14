// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "devtools_delegate.h"

namespace brightray {

DevToolsDelegate::DevToolsDelegate() {
}

DevToolsDelegate::~DevToolsDelegate() {
}

std::string DevToolsDelegate::GetDiscoveryPageHTML() {
  return std::string();
}

bool DevToolsDelegate::BundlesFrontendResources() {
  return true;
}

base::FilePath DevToolsDelegate::GetDebugFrontendDir() {
  return base::FilePath();
}

std::string DevToolsDelegate::GetPageThumbnailData(const GURL&) {
  return std::string();
}

content::RenderViewHost* DevToolsDelegate::CreateNewTarget() {
  return nullptr;
}

content::DevToolsHttpHandlerDelegate::TargetType DevToolsDelegate::GetTargetType(content::RenderViewHost*) {
  return kTargetTypeTab;
}

}
