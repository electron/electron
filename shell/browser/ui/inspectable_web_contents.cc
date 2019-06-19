// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2013 Adam Roben <adam@roben.org>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "atom/browser/ui/inspectable_web_contents.h"

#include "atom/browser/ui/inspectable_web_contents_impl.h"

namespace atom {

InspectableWebContents* InspectableWebContents::Create(
    content::WebContents* web_contents,
    PrefService* pref_service,
    bool is_guest) {
  return new InspectableWebContentsImpl(web_contents, pref_service, is_guest);
}

}  // namespace atom
