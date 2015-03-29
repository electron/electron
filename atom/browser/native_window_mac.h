// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NATIVE_WINDOW_MAC_H_
#define ATOM_BROWSER_NATIVE_WINDOW_MAC_H_

#import <Cocoa/Cocoa.h>

#include <string>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "base/memory/scoped_ptr.h"
#include "atom/browser/native_window.h"

@class AtomNSWindow;
@class AtomNSWindowDelegate;
@class FullSizeContentView;
class SkRegion;

namespace atom {

class NativeWindowMac : public NativeWindow {
 public:
  explicit NativeWindowMac(content::WebContents* web_contents,
                           const mate::Dictionary& options);
  virtual ~NativeWindowMac();

  // NativeWindow implementation.
  void Close() override;
  void CloseImmediately() override;
  void Move(const gfx::Rect& pos) override;
  void Focus(bool focus) override;
  bool IsFocused() override;
  void Show() override;
  void ShowInactive() override;
  void Hide() override;
  bool IsVisible() override;
  void Maximize() override;
  void Unmaximize() override;
  bool IsMaximized() override;
  void Minimize() override;
  void Restore() override;
  bool IsMinimized() override;
  void SetFullScreen(bool fullscreen) override;
  bool IsFullscreen() override;
  void SetSize(const gfx::Size& size) override;
  gfx::Size GetSize() override;
  void SetContentSize(const gfx::Size& size) override;
  gfx::Size GetContentSize() override;
  void SetMinimumSize(const gfx::Size& size) override;
  gfx::Size GetMinimumSize() override;
  void SetMaximumSize(const gfx::Size& size) override;
  gfx::Size GetMaximumSize() override;
  void SetResizable(bool resizable) override;
  bool IsResizable() override;
  void SetAlwaysOnTop(bool top) override;
  bool IsAlwaysOnTop() override;
  void Center() override;
  void SetPosition(const gfx::Point& position) override;
  gfx::Point GetPosition() override;
  void SetTitle(const std::string& title) override;
  std::string GetTitle() override;
  void FlashFrame(bool flash) override;
  void SetSkipTaskbar(bool skip) override;
  void SetKiosk(bool kiosk) override;
  bool IsKiosk() override;
  void SetRepresentedFilename(const std::string& filename) override;
  std::string GetRepresentedFilename() override;
  void SetDocumentEdited(bool edited) override;
  bool IsDocumentEdited() override;
  bool HasModalDialog() override;
  gfx::NativeWindow GetNativeWindow() override;
  void SetProgressBar(double progress) override;
  void SetOverlayIcon(const gfx::Image& overlay,
                      const std::string& description) override;
  void ShowDefinitionForSelection() override;

  void SetVisibleOnAllWorkspaces(bool visible) override;
  bool IsVisibleOnAllWorkspaces() override;

  // Returns true if |point| in local Cocoa coordinate system falls within
  // the draggable region.
  bool IsWithinDraggableRegion(NSPoint point) const;

  // Called to handle a mouse event.
  void HandleMouseEvent(NSEvent* event);

  // Clip web view to rounded corner.
  void ClipWebView();

 protected:
  void UpdateDraggableRegions(
      const std::vector<DraggableRegion>& regions) override;

  // Implementations of content::WebContentsDelegate.
  void HandleKeyboardEvent(
      content::WebContents*,
      const content::NativeWebKeyboardEvent&) override;

 private:
  void InstallView();
  void UninstallView();

  // Install the drag view, which will cover the whole window and decides
  // whehter we can drag.
  void InstallDraggableRegionView();

  base::scoped_nsobject<AtomNSWindow> window_;
  base::scoped_nsobject<AtomNSWindowDelegate> window_delegate_;

  // The view that will fill the whole frameless window.
  base::scoped_nsobject<FullSizeContentView> content_view_;

  bool is_kiosk_;

  NSInteger attention_request_id_;  // identifier from requestUserAttention

  // The presentation options before entering kiosk mode.
  NSApplicationPresentationOptions kiosk_options_;

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
