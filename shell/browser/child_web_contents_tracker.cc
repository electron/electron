// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/child_web_contents_tracker.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

ChildWebContentsTracker::ChildWebContentsTracker(
    content::WebContents* web_contents)
    : content::WebContentsUserData<ChildWebContentsTracker>(*web_contents) {}

ChildWebContentsTracker::~ChildWebContentsTracker() = default;

WEB_CONTENTS_USER_DATA_KEY_IMPL(ChildWebContentsTracker);

}  // namespace electron
