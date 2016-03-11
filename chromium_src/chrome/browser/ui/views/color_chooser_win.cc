// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/color_chooser_aura.h"
#include "chrome/browser/ui/views/color_chooser_dialog.h"
#include "content/public/browser/color_chooser.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "ui/aura/window.h"
#include "ui/views/color_chooser/color_chooser_listener.h"

class ColorChooserWin : public content::ColorChooser,
                        public views::ColorChooserListener {
 public:
  static ColorChooserWin* Open(content::WebContents* web_contents,
                               SkColor initial_color);

  ColorChooserWin(content::WebContents* web_contents,
                  SkColor initial_color);
  ~ColorChooserWin();

  // content::ColorChooser overrides:
  virtual void End() override;
  virtual void SetSelectedColor(SkColor color) override {}

  // views::ColorChooserListener overrides:
  virtual void OnColorChosen(SkColor color);
  virtual void OnColorChooserDialogClosed();

 private:
  static ColorChooserWin* current_color_chooser_;

  // The web contents invoking the color chooser.  No ownership. because it will
  // outlive this class.
  content::WebContents* web_contents_;

  // The color chooser dialog which maintains the native color chooser UI.
  scoped_refptr<ColorChooserDialog> color_chooser_dialog_;
};

ColorChooserWin* ColorChooserWin::current_color_chooser_ = NULL;

ColorChooserWin* ColorChooserWin::Open(content::WebContents* web_contents,
                                       SkColor initial_color) {
  if (current_color_chooser_)
    return NULL;
  current_color_chooser_ = new ColorChooserWin(web_contents, initial_color);
  return current_color_chooser_;
}

ColorChooserWin::ColorChooserWin(content::WebContents* web_contents,
                                 SkColor initial_color)
    : web_contents_(web_contents) {
  gfx::NativeWindow owning_window = web_contents->GetRenderViewHost()
                                                ->GetWidget()
                                                ->GetView()
                                                ->GetNativeView()
                                                ->GetToplevelWindow();
  color_chooser_dialog_ = new ColorChooserDialog(this,
                                                 initial_color,
                                                 owning_window);
}

ColorChooserWin::~ColorChooserWin() {
  // Always call End() before destroying.
  DCHECK(!color_chooser_dialog_);
}

void ColorChooserWin::End() {
  // The ColorChooserDialog's listener is going away.  Ideally we'd
  // programmatically close the dialog at this point.  Since that's impossible,
  // we instead tell the dialog its listener is going away, so that the dialog
  // doesn't try to communicate with a destroyed listener later.  (We also tell
  // the renderer the dialog is closed, since from the renderer's perspective
  // it effectively is.)
  OnColorChooserDialogClosed();
}

void ColorChooserWin::OnColorChosen(SkColor color) {
  if (web_contents_)
    web_contents_->DidChooseColorInColorChooser(color);
}

void ColorChooserWin::OnColorChooserDialogClosed() {
  if (color_chooser_dialog_.get()) {
    color_chooser_dialog_->ListenerDestroyed();
    color_chooser_dialog_ = NULL;
  }
  DCHECK(current_color_chooser_ == this);
  current_color_chooser_ = NULL;
  if (web_contents_)
    web_contents_->DidEndColorChooser();
}

namespace chrome {

content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color) {
  return ColorChooserWin::Open(web_contents, initial_color);
}

}  // namespace chrome
