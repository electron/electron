// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "common/content_client.h"

#include "common/application_info.h"

#include "base/stringprintf.h"
#include "base/string_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "webkit/user_agent/user_agent_util.h"

namespace brightray {

ContentClient::ContentClient() {
}

ContentClient::~ContentClient() {
}

std::string ContentClient::GetProduct() const {
  auto name = GetApplicationName();
  RemoveChars(name, kWhitespaceASCII, &name);
  return base::StringPrintf("%s/%s", name.c_str(), GetApplicationVersion().c_str());
}

std::string ContentClient::GetUserAgent() const {
  return webkit_glue::BuildUserAgentFromProduct(GetProduct());
}

base::StringPiece ContentClient::GetDataResource(int resource_id, ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(resource_id, scale_factor);
}

}