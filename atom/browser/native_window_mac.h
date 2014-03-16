// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_MAC_H_
#define ATOM_BROWSER_NATIVE_WINDOW_MAC_H_

#import <Cocoa/Cocoa.h>

#include "base/memory/scoped_ptr.h"
#include "atom/browser/native_window.h"

class SkRegion;

namespace atom {

class NativeWindowMac : public NativeWindow {
 public:
  explicit NativeWindowMac(content::WebContents* web_contents,
                           base::DictionaryValue* options);
  virtual ~NativeWindowMac();

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

  void NotifyWindowBlur() { NativeWindow::NotifyWindowBlur(); }

  // Returns true if |point| in local Cocoa coordinate system falls within
  // the draggable region.
  bool IsWithinDraggableRegion(NSPoint point) const;

  // Called to handle a mouse event.
  void HandleMouseEvent(NSEvent* event);

  // Clip web view to rounded corner.
  void ClipWebView();

  NSWindow*& window() { return window_; }
  SkRegion* draggable_region() const { return draggable_region_.get(); }

 protected:
  virtual void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) OVERRIDE;

  // Implementations of content::WebContentsDelegate.
  virtual void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent&) OVERRIDE;

 private:
  void InstallView();
  void UninstallView();
  void InstallDraggableRegionViews();
  void UpdateDraggableRegionsForCustomDrag(
      const std::vector<DraggableRegion>& regions);

  NSWindow* window_;

  bool is_kiosk_;

  NSInteger attention_request_id_;  // identifier from requestUserAttention

  // For system drag, the whole window is draggable and the non-draggable areas
  // have to been explicitly excluded.
  std::vector<gfx::Rect> system_drag_exclude_areas_;

  // For custom drag, the whole window is non-draggable and the draggable region
  // has to been explicitly provided.
  scoped_ptr<SkRegion> draggable_region_;  // used in custom drag.

  // Mouse location since the last mouse event, in screen coordinates. This is
  // used in custom drag to compute the window movement.
  NSPoint last_mouse_offset_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowMac);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NATIVE_WINDOW_MAC_H_
