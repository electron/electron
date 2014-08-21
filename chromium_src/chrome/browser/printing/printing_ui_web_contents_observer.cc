// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/printing_ui_web_contents_observer.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

PrintingUIWebContentsObserver::PrintingUIWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

gfx::NativeView PrintingUIWebContentsObserver::GetParentView() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  return web_contents() ? web_contents()->GetNativeView() : NULL;
}
