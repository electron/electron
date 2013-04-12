// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_main_parts.h"

#import <AppKit/AppKit.h>

#include "brightray/browser/browser_context.h"
#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"

namespace atom {

void AtomBrowserMainParts::PreMainMessageLoopRun() {
  brightray::BrowserMainParts::PreMainMessageLoopRun();

  auto contentRect = NSMakeRect(0, 0, 800, 600);
  auto styleMask = NSTitledWindowMask |
                   NSClosableWindowMask |
                   NSMiniaturizableWindowMask |
                   NSResizableWindowMask;
  auto window = [[NSWindow alloc] initWithContentRect:contentRect
                                            styleMask:styleMask
                                              backing:NSBackingStoreBuffered
                                                defer:YES];
  window.title = @"Atom";

  // FIXME: We're leaking this object (see #3).
  auto contents = brightray::InspectableWebContents::Create(content::WebContents::CreateParams(browser_context()));
  // FIXME: And this one!
  contents->GetWebContents()->SetDelegate(
      new brightray::DefaultWebContentsDelegate());
  auto contentsView = contents->GetView()->GetNativeView();

  contentsView.frame = [window.contentView bounds];
  contentsView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

  [window.contentView addSubview:contentsView];
  [window makeFirstResponder:contentsView];
  [window makeKeyAndOrderFront:nil];

  contents->GetWebContents()->GetController().LoadURL(
      GURL("http://adam.roben.org/brightray_example/start.html"),
      content::Referrer(),
      content::PAGE_TRANSITION_AUTO_TOPLEVEL,
      std::string());
}

}  // namespace atom
