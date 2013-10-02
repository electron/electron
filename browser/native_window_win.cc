// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/native_window_win.h"

#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "browser/ui/win/menu_2.h"
#include "common/draggable_region.h"
#include "common/options_switches.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_view.h"
#include "ui/gfx/path.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/native_frame_view.h"

namespace atom {

namespace {

const int kResizeInsideBoundsSize = 5;
const int kResizeAreaCornerSize = 16;

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         views::View* contents_view,
                         NativeWindowWin* shell)
      : views::ClientView(widget, contents_view),
        shell_(shell) {
  }
  virtual ~NativeWindowClientView() {}

  virtual bool CanClose() OVERRIDE {
    shell_->CloseWebContents();
    return false;
  }

 private:
  NativeWindowWin* shell_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

class NativeWindowFrameView : public views::NativeFrameView {
 public:
  explicit NativeWindowFrameView(views::Widget* frame, NativeWindowWin* shell)
      : NativeFrameView(frame),
        shell_(shell) {
  }
  virtual ~NativeWindowFrameView() {}

  virtual gfx::Size GetMinimumSize() OVERRIDE {
    return shell_->GetMinimumSize();
  }

  virtual gfx::Size GetMaximumSize() OVERRIDE {
    return shell_->GetMaximumSize();
  }

 private:
  NativeWindowWin* shell_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowFrameView);
};

class NativeWindowFramelessView : public views::NonClientFrameView {
 public:
  explicit NativeWindowFramelessView(views::Widget* frame,
                                     NativeWindowWin* shell)
      : frame_(frame),
        shell_(shell) {
  }
  virtual ~NativeWindowFramelessView() {}

  // views::NonClientFrameView implementations:
  virtual gfx::Rect NativeWindowFramelessView::GetBoundsForClientView() const
      OVERRIDE {
    return bounds();
  }

  virtual gfx::Rect NativeWindowFramelessView::GetWindowBoundsForClientBounds(
        const gfx::Rect& client_bounds) const OVERRIDE {
    gfx::Rect window_bounds = client_bounds;
    // Enforce minimum size (1, 1) in case that client_bounds is passed with
    // empty size. This could occur when the frameless window is being
    // initialized.
    if (window_bounds.IsEmpty()) {
      window_bounds.set_width(1);
      window_bounds.set_height(1);
    }
    return window_bounds;
  }

  virtual int NonClientHitTest(const gfx::Point& point) OVERRIDE {
    if (frame_->IsFullscreen())
      return HTCLIENT;

    // Check the frame first, as we allow a small area overlapping the contents
    // to be used for resize handles.
    bool can_ever_resize = frame_->widget_delegate() ?
        frame_->widget_delegate()->CanResize() :
        false;
    // Don't allow overlapping resize handles when the window is maximized or
    // fullscreen, as it can't be resized in those states.
    int resize_border =
        frame_->IsMaximized() || frame_->IsFullscreen() ? 0 :
        kResizeInsideBoundsSize;
    int frame_component = GetHTComponentForFrame(point,
                                                 resize_border,
                                                 resize_border,
                                                 kResizeAreaCornerSize,
                                                 kResizeAreaCornerSize,
                                                 can_ever_resize);
    if (frame_component != HTNOWHERE)
      return frame_component;

    // Check for possible draggable region in the client area for the frameless
    // window.
    if (shell_->draggable_region() &&
        shell_->draggable_region()->contains(point.x(), point.y()))
      return HTCAPTION;

    int client_component = frame_->client_view()->NonClientHitTest(point);
    if (client_component != HTNOWHERE)
      return client_component;

    // Caption is a safe default.
    return HTCAPTION;
  }

  virtual void GetWindowMask(const gfx::Size& size,
                             gfx::Path* window_mask) OVERRIDE {}
  virtual void ResetWindowControls() OVERRIDE {}
  virtual void UpdateWindowIcon() OVERRIDE {}
  virtual void UpdateWindowTitle() OVERRIDE {}

  // views::View implementations:
  virtual gfx::Size NativeWindowFramelessView::GetPreferredSize() OVERRIDE {
    gfx::Size pref = frame_->client_view()->GetPreferredSize();
    gfx::Rect bounds(0, 0, pref.width(), pref.height());
    return frame_->non_client_view()->GetWindowBoundsForClientBounds(
        bounds).size();
  }

  virtual gfx::Size GetMinimumSize() OVERRIDE {
    return shell_->GetMinimumSize();
  }

  virtual gfx::Size GetMaximumSize() OVERRIDE {
    return shell_->GetMaximumSize();
  }

 private:
  views::Widget* frame_;
  NativeWindowWin* shell_;

  DISALLOW_COPY_AND_ASSIGN(NativeWindowFramelessView);
};

}  // namespace

NativeWindowWin::NativeWindowWin(content::WebContents* web_contents,
                                 base::DictionaryValue* options)
    : NativeWindow(web_contents, options),
      window_(new views::Widget),
      web_view_(new views::WebView(NULL)),
      resizable_(true) {
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_WINDOW);
  params.delegate = this;
  params.remove_standard_frame = !has_frame_;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  window_->set_frame_type(views::Widget::FRAME_TYPE_FORCE_NATIVE);
  window_->Init(params);

  int width = 800, height = 600;
  options->GetInteger(switches::kWidth, &width);
  options->GetInteger(switches::kHeight, &height);

  gfx::Size size(width, height);
  window_->CenterWindow(size);

  web_view_->SetWebContents(web_contents);
  OnViewWasResized();
}

NativeWindowWin::~NativeWindowWin() {
}

void NativeWindowWin::Close() {
  window_->Close();
}

void NativeWindowWin::CloseImmediately() {
  window_->CloseNow();
}

void NativeWindowWin::Move(const gfx::Rect& bounds) {
  window_->SetBounds(bounds);
}

void NativeWindowWin::Focus(bool focus) {
  if (focus)
    window_->Activate();
  else
    window_->Deactivate();
}

bool NativeWindowWin::IsFocused() {
  return window_->IsActive();
}

void NativeWindowWin::Show() {
  window_->Show();
}

void NativeWindowWin::Hide() {
  window_->Hide();
}

void NativeWindowWin::Maximize() {
  window_->Maximize();
}

void NativeWindowWin::Unmaximize() {
  window_->Restore();
}

void NativeWindowWin::Minimize() {
  window_->Minimize();
}

void NativeWindowWin::Restore() {
  window_->Restore();
}

void NativeWindowWin::SetFullscreen(bool fullscreen) {
  window_->SetFullscreen(fullscreen);
}

bool NativeWindowWin::IsFullscreen() {
  return window_->IsFullscreen();
}

void NativeWindowWin::SetSize(const gfx::Size& size) {
  window_->SetSize(size);
}

gfx::Size NativeWindowWin::GetSize() {
  return window_->GetWindowBoundsInScreen().size();
}

void NativeWindowWin::SetMinimumSize(const gfx::Size& size) {
  minimum_size_ = size;
}

gfx::Size NativeWindowWin::GetMinimumSize() {
  return minimum_size_;
}

void NativeWindowWin::SetMaximumSize(const gfx::Size& size) {
  maximum_size_ = size;
}

gfx::Size NativeWindowWin::GetMaximumSize() {
  return maximum_size_;
}

void NativeWindowWin::SetResizable(bool resizable) {
  resizable_ = resizable;
}

bool NativeWindowWin::IsResizable() {
  return resizable_;
}

void NativeWindowWin::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
}

bool NativeWindowWin::IsAlwaysOnTop() {
  DWORD style = ::GetWindowLong(window_->GetNativeView(), GWL_EXSTYLE);
  return style & WS_EX_TOPMOST;
}

void NativeWindowWin::Center() {
  window_->CenterWindow(GetSize());
}

void NativeWindowWin::SetPosition(const gfx::Point& position) {
  window_->SetBounds(gfx::Rect(position, GetSize()));
}

gfx::Point NativeWindowWin::GetPosition() {
  return window_->GetWindowBoundsInScreen().origin();
}

void NativeWindowWin::SetTitle(const std::string& title) {
  title_ = UTF8ToUTF16(title);
  window_->UpdateWindowTitle();
}

std::string NativeWindowWin::GetTitle() {
  return UTF16ToUTF8(title_);
}

void NativeWindowWin::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void NativeWindowWin::SetKiosk(bool kiosk) {
  SetFullscreen(kiosk);
}

bool NativeWindowWin::IsKiosk() {
  return IsFullscreen();
}

gfx::NativeWindow NativeWindowWin::GetNativeWindow() {
  return window_->GetNativeView();
}

void NativeWindowWin::SetMenu(ui::MenuModel* menu_model) {
  menu_.reset(new atom::Menu2(menu_model, true));
  ::SetMenu(GetNativeWindow(), menu_->GetNativeMenu());
}

void NativeWindowWin::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  if (has_frame_)
    return;

  SkRegion* draggable_region = new SkRegion;

  // By default, the whole window is non-draggable. We need to explicitly
  // include those draggable regions.
  for (std::vector<DraggableRegion>::const_iterator iter = regions.begin();
       iter != regions.end(); ++iter) {
    const DraggableRegion& region = *iter;
    draggable_region->op(
        region.bounds.x(),
        region.bounds.y(),
        region.bounds.right(),
        region.bounds.bottom(),
        region.draggable ? SkRegion::kUnion_Op : SkRegion::kDifference_Op);
  }

  draggable_region_.reset(draggable_region);
  OnViewWasResized();
}

void NativeWindowWin::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  // Any unhandled keyboard/character messages should be defproced.
  // This allows stuff like F10, etc to work correctly.
  DefWindowProc(event.os_event.hwnd, event.os_event.message,
                event.os_event.wParam, event.os_event.lParam);
}

bool NativeWindowWin::CanResize() const {
  return resizable_;
}

bool NativeWindowWin::CanMaximize() const {
  return resizable_;
}

string16 NativeWindowWin::GetWindowTitle() const {
  return title_;
}

bool NativeWindowWin::ShouldHandleSystemCommands() const {
  return true;
}

bool NativeWindowWin::ShouldShowWindowIcon() const {
  return true;
}

views::Widget* NativeWindowWin::GetWidget() {
  return window_.get();
}

const views::Widget* NativeWindowWin::GetWidget() const {
  return window_.get();
}

views::ClientView* NativeWindowWin::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, web_view_, this);
}

views::NonClientFrameView* NativeWindowWin::CreateNonClientFrameView(
    views::Widget* widget) {
  if (has_frame_)
    return new NativeWindowFrameView(widget, this);

  return new NativeWindowFramelessView(widget, this);
}

void NativeWindowWin::OnViewWasResized() {
  // Set the window shape of the RWHV.
  gfx::Size sz = web_view_->size();
  int height = sz.height(), width = sz.width();
  gfx::Path path;
  path.addRect(0, 0, width, height);
  SetWindowRgn(web_contents()->GetView()->GetNativeView(),
               path.CreateNativeRegion(),
               1);

  SkRegion* rgn = new SkRegion;
  if (!window_->IsFullscreen() && !window_->IsMaximized()) {
    if (draggable_region())
      rgn->op(*draggable_region(), SkRegion::kUnion_Op);

    if (!has_frame_ && CanResize()) {
      rgn->op(0, 0, width, kResizeInsideBoundsSize, SkRegion::kUnion_Op);
      rgn->op(0, 0, kResizeInsideBoundsSize, height, SkRegion::kUnion_Op);
      rgn->op(width - kResizeInsideBoundsSize, 0, width, height,
          SkRegion::kUnion_Op);
      rgn->op(0, height - kResizeInsideBoundsSize, width, height,
          SkRegion::kUnion_Op);
    }
  }

  content::WebContents* web_contents = GetWebContents();
  if (web_contents->GetRenderViewHost()->GetView())
    web_contents->GetRenderViewHost()->GetView()->SetClickthroughRegion(rgn);
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   base::DictionaryValue* options) {
  return new NativeWindowWin(web_contents, options);
}

}  // namespace atom
