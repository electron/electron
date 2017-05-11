// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_COMMON_CONTENT_CLIENT_H_
#define BRIGHTRAY_COMMON_CONTENT_CLIENT_H_

#include "base/compiler_specific.h"
#include "content/public/common/content_client.h"

namespace brightray {

std::string GetBrightrayUserAgent();

class ContentClient : public content::ContentClient {
 public:
  ContentClient();
  ~ContentClient();

 private:
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(int resource_id,
                                    ui::ScaleFactor) const override;
  gfx::Image& GetNativeImageNamed(int resource_id) const override;
  base::RefCountedMemory* GetDataResourceBytes(int resource_id) const override;

  DISALLOW_COPY_AND_ASSIGN(ContentClient);
};

}  // namespace brightray

#endif
