// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_GTK_H_
#define ATOM_BROWSER_NATIVE_WINDOW_GTK_H_

#include <gtk/gtk.h>

#include "browser/native_window.h"
#include "ui/base/gtk/gtk_signal.h"
#include "ui/gfx/size.h"

namespace atom {

class NativeWindowGtk : public NativeWindow {
 public:
  explicit NativeWindowGtk(content::WebContents* web_contents,
                           base::DictionaryValue* options);
  virtual ~NativeWindowGtk();

  // NativeWindow implementation.
  virtual void Close() OVERRIDE;
  virtual void CloseImmediately() OVERRIDE;
  virtual void Move(const gfx::Rect& pos) OVERRIDE;
  virtual void Focus(bool focus) OVERRIDE;
  virtual bool IsFocused() OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;
  virtual bool IsVisible() OVERRIDE;
  virtual void Maximize() OVERRIDE;
  virtual void Unmaximize() OVERRIDE;
  virtual void Minimize() OVERRIDE;
  virtual void Restore() OVERRIDE;
  virtual void SetFullscreen(bool fullscreen) OVERRIDE;
  virtual bool IsFullscreen() OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetSize() OVERRIDE;
  virtual void SetMinimumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMinimumSize() OVERRIDE;
  virtual void SetMaximumSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;
  virtual void SetResizable(bool resizable) OVERRIDE;
  virtual bool IsResizable() OVERRIDE;
  virtual void SetAlwaysOnTop(bool top) OVERRIDE;
  virtual bool IsAlwaysOnTop() OVERRIDE;
  virtual void Center() OVERRIDE;
  virtual void SetPosition(const gfx::Point& position) OVERRIDE;
  virtual gfx::Point GetPosition() OVERRIDE;
  virtual void SetTitle(const std::string& title) OVERRIDE;
  virtual std::string GetTitle() OVERRIDE;
  virtual void FlashFrame(bool flash) OVERRIDE;
  virtual void SetKiosk(bool kiosk) OVERRIDE;
  virtual bool IsKiosk() OVERRIDE;
  virtual bool HasModalDialog() OVERRIDE;
  virtual gfx::NativeWindow GetNativeWindow() OVERRIDE;

 protected:
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) OVERRIDE;

 private:
  // Set WebKit's style from current theme.
  void SetWebKitColorStyle();

  CHROMEGTK_CALLBACK_1(NativeWindowGtk, gboolean, OnWindowDeleteEvent,
                       GdkEvent*);

  GtkWindow* window_;

  bool fullscreen_;
  bool is_always_on_top_;
  gfx::Size minimum_size_;
  gfx::Size maximum_size_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowGtk);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_GTK_H_
