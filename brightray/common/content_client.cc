// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "common/content_client.h"

#include "common/application_info.h"

#include "base/strings/stringprintf.h"
#include "base/strings/string_util.h"
#include "content/public/common/user_agent.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace brightray {

std::string GetProductInternal() {
  auto name = GetApplicationName();
  base::RemoveChars(name, base::kWhitespaceASCII, &name);
  return base::StringPrintf("%s/%s",
      name.c_str(), GetApplicationVersion().c_str());
}

std::string GetBrightrayUserAgent() {
  return content::BuildUserAgentFromProduct(GetProductInternal());
}

ContentClient::ContentClient() {
}

ContentClient::~ContentClient() {
}

std::string ContentClient::GetProduct() const {
  return GetProductInternal();
}

std::string ContentClient::GetUserAgent() const {
  return GetBrightrayUserAgent();
}

base::string16 ContentClient::GetLocalizedString(int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece ContentClient::GetDataResource(
    int resource_id, ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

gfx::Image& ContentClient::GetNativeImageNamed(int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

base::RefCountedMemory* ContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ResourceBundle::GetSharedInstance().LoadDataResourceBytes(resource_id);
}

}  // namespace brightray
