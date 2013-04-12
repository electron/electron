// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/native_window.h"

#include <string>

#include "base/values.h"
#include "brightray/browser/browser_context.h"
#include "brightray/browser/default_web_contents_delegate.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "common/options_switches.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"

namespace atom {

NativeWindow::NativeWindow(content::BrowserContext* browser_context,
                           base::DictionaryValue* options)
    : inspectable_web_contents_(brightray::InspectableWebContents::Create(
          content::WebContents::CreateParams(browser_context))) {
  GetWebContents()->SetDelegate(new brightray::DefaultWebContentsDelegate());
}

NativeWindow::~NativeWindow() {
}

void NativeWindow::InitFromOptions(base::DictionaryValue* options) {
  // Setup window from options.
  int x, y;
  std::string position;
  if (options->GetInteger(switches::kX, &x) &&
      options->GetInteger(switches::kY, &y)) {
    int width, height;
    options->GetInteger(switches::kWidth, &width);
    options->GetInteger(switches::kHeight, &height);
    Move(gfx::Rect(x, y, width, height));
  } else if (options->GetString(switches::kPosition, &position)) {
    SetPosition(position);
  }
  int min_height, min_width;
  if (options->GetInteger(switches::kMinHeight, &min_height) &&
      options->GetInteger(switches::kMinWidth, &min_width)) {
    SetMinimumSize(min_width, min_height);
  }
  int max_height, max_width;
  if (options->GetInteger(switches::kMaxHeight, &max_height) &&
      options->GetInteger(switches::kMaxWidth, &max_width)) {
    SetMaximumSize(max_width, max_height);
  }
  bool resizable;
  if (options->GetBoolean(switches::kResizable, &resizable)) {
    SetResizable(resizable);
  }
  bool top;
  if (options->GetBoolean(switches::kAlwaysOnTop, &top) && top) {
    SetAlwaysOnTop(true);
  }
  bool fullscreen;
  if (options->GetBoolean(switches::kFullscreen, &fullscreen) && fullscreen) {
    SetFullscreen(true);
  }
  bool kiosk;
  if (options->GetBoolean(switches::kKiosk, &kiosk) && kiosk) {
    SetKiosk(kiosk);
  }
  std::string title("Atom Shell");
  options->GetString(switches::kTitle, &title);
  SetTitle(title);

  // Then show it.
  bool show = true;
  options->GetBoolean(switches::kShow, &show);
  if (show)
    Show();
}

void NativeWindow::ShowDevTools() {
  inspectable_web_contents()->ShowDevTools();
}

void NativeWindow::CloseDevTools() {
  inspectable_web_contents()->GetView()->CloseDevTools();
}

content::WebContents* NativeWindow::GetWebContents() const {
  return inspectable_web_contents_->GetWebContents();
}

}  // namespace atom
