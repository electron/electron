// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_aura.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/aura/env.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"

namespace atom {

namespace {

class FillLayout : public aura::LayoutManager {
 public:
  explicit FillLayout(aura::Window* root)
      : root_(root) {
  }

  virtual ~FillLayout() {}

 private:
  // aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE {
  }

  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE {
    child->SetBounds(root_->bounds());
  }

  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE {
  }

  virtual void OnWindowRemovedFromLayout(aura::Window* child) OVERRIDE {
  }

  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) OVERRIDE {
  }

  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* root_;

  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

}  // namespace

NativeWindowAura::NativeWindowAura(content::WebContents* web_contents,
                                   const mate::Dictionary& options)
    : NativeWindow(web_contents, options) {
  aura::Env::CreateInstance();

  host_.reset(aura::WindowTreeHost::Create(gfx::Rect(gfx::Size(800, 600))));
  host_->InitHost();
  host_->window()->SetLayoutManager(new FillLayout(host_->window()));
  host_->window()->AddChild(web_contents->GetView()->GetNativeView());
}

NativeWindowAura::~NativeWindowAura() {
}

void NativeWindowAura::Close() {
}

void NativeWindowAura::CloseImmediately() {
}

void NativeWindowAura::Move(const gfx::Rect& pos) {
}

void NativeWindowAura::Focus(bool focus) {
}

bool NativeWindowAura::IsFocused() {
  return false;
}

void NativeWindowAura::Show() {
  host_->Show();
}

void NativeWindowAura::Hide() {
  host_->Hide();
}

bool NativeWindowAura::IsVisible() {
  return false;
}

void NativeWindowAura::Maximize() {
}

void NativeWindowAura::Unmaximize() {
}

void NativeWindowAura::Minimize() {
}

void NativeWindowAura::Restore() {
}

void NativeWindowAura::SetFullscreen(bool fullscreen) {
}

bool NativeWindowAura::IsFullscreen() {
  return false;
}

void NativeWindowAura::SetSize(const gfx::Size& size) {
}

gfx::Size NativeWindowAura::GetSize() {
  return gfx::Size();
}

void NativeWindowAura::SetContentSize(const gfx::Size& size) {
}

gfx::Size NativeWindowAura::GetContentSize() {
  return gfx::Size();
}

void NativeWindowAura::SetMinimumSize(const gfx::Size& size) {
}

gfx::Size NativeWindowAura::GetMinimumSize() {
  return gfx::Size();
}

void NativeWindowAura::SetMaximumSize(const gfx::Size& size) {
}

gfx::Size NativeWindowAura::GetMaximumSize() {
  return gfx::Size();
}

void NativeWindowAura::SetResizable(bool resizable) {
}

bool NativeWindowAura::IsResizable() {
  return false;
}

void NativeWindowAura::SetAlwaysOnTop(bool top) {
}

bool NativeWindowAura::IsAlwaysOnTop() {
  return false;
}

void NativeWindowAura::Center() {
}

void NativeWindowAura::SetPosition(const gfx::Point& position) {
}

gfx::Point NativeWindowAura::GetPosition() {
  return gfx::Point(0, 0);
}

void NativeWindowAura::SetTitle(const std::string& title) {
}

std::string NativeWindowAura::GetTitle() {
  return "";
}

void NativeWindowAura::FlashFrame(bool flash) {
}

void NativeWindowAura::SetSkipTaskbar(bool skip) {
}

void NativeWindowAura::SetKiosk(bool kiosk) {
}

bool NativeWindowAura::IsKiosk() {
  return false;
}

gfx::NativeWindow NativeWindowAura::GetNativeWindow() {
  return NULL;
}

void NativeWindowAura::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   const mate::Dictionary& options) {
  return new NativeWindowAura(web_contents, options);
}

}  // namespace atom
