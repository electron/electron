// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/color_chooser_aura.h"

#include "chrome/browser/ui/browser_dialogs.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/color_chooser/color_chooser_view.h"
#include "ui/views/widget/widget.h"

ColorChooserAura::ColorChooserAura(content::WebContents* web_contents,
                                   SkColor initial_color)
    : web_contents_(web_contents) {
  view_ = new views::ColorChooserView(this, initial_color);
  widget_ = views::Widget::CreateWindowWithParent(
      view_, web_contents->GetTopLevelNativeWindow());
  widget_->Show();
}

void ColorChooserAura::OnColorChosen(SkColor color) {
  if (web_contents_)
    web_contents_->DidChooseColorInColorChooser(color);
}

void ColorChooserAura::OnColorChooserDialogClosed() {
  view_ = NULL;
  widget_ = NULL;
  DidEndColorChooser();
}

void ColorChooserAura::End() {
  if (widget_) {
    view_->set_listener(NULL);
    widget_->Close();
    view_ = NULL;
    widget_ = NULL;
    // DidEndColorChooser will invoke Browser::DidEndColorChooser, which deletes
    // this. Take care of the call order.
    DidEndColorChooser();
  }
}

void ColorChooserAura::DidEndColorChooser() {
  if (web_contents_)
    web_contents_->DidEndColorChooser();
}

void ColorChooserAura::SetSelectedColor(SkColor color) {
  if (view_)
    view_->OnColorChanged(color);
}

// static
ColorChooserAura* ColorChooserAura::Open(
    content::WebContents* web_contents, SkColor initial_color) {
  return new ColorChooserAura(web_contents, initial_color);
}

#if !defined(OS_WIN)
namespace chrome {

content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color) {
  return ColorChooserAura::Open(web_contents, initial_color);
}

}  // namespace chrome
#endif  // OS_WIN
