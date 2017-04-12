// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/child_web_contents_tracker.h"

#include <unordered_set>

namespace atom {

namespace {

std::unordered_set<content::WebContents*> g_child_web_contents;

}  // namespace

ChildWebContentsTracker::ChildWebContentsTracker(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  g_child_web_contents.insert(web_contents);
}

bool ChildWebContentsTracker::IsChildWebContents(
    content::WebContents* web_contents) {
  return g_child_web_contents.find(web_contents) != g_child_web_contents.end();
}

void ChildWebContentsTracker::WebContentsDestroyed() {
  g_child_web_contents.erase(web_contents());
  delete this;
}

}  // namespace atom
