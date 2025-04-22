// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_
#define ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_

#include <memory>

namespace content {
class WebContents;
class WebContentsViewDelegate;
}  // namespace content

std::unique_ptr<content::WebContentsViewDelegate> CreateWebContentsViewDelegate(content::WebContents* web_contents);

#endif  // ELECTRON_WEB_CONTENTS_VIEW_DELEGATE_H_
