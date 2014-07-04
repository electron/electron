// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_views.h"

#include <string>
#include <vector>

#include "atom/common/options_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents_view.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/window/client_view.h"
#include "ui/views/window/custom_frame_view.h"
#include "ui/views/widget/widget.h"

namespace atom {

namespace {

class NativeWindowClientView : public views::ClientView {
 public:
  NativeWindowClientView(views::Widget* widget,
                         NativeWindowViews* contents_view)
      : views::ClientView(widget, contents_view) {
  }
  virtual ~NativeWindowClientView() {}

  virtual bool CanClose() OVERRIDE {
    static_cast<NativeWindowViews*>(contents_view())->CloseWebContents();
    return false;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(NativeWindowClientView);
};

}  // namespace

NativeWindowViews::NativeWindowViews(content::WebContents* web_contents,
                                     const mate::Dictionary& options)
    : NativeWindow(web_contents, options),
      window_(new views::Widget),
      web_view_(new views::WebView(web_contents->GetBrowserContext())),
      resizable_(true) {
  options.Get(switches::kResizable, &resizable_);
  options.Get(switches::kTitle, &title_);

  views::Widget::InitParams params;
  params.delegate = this;
  params.type = has_frame_ ? views::Widget::InitParams::TYPE_WINDOW :
                             views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.top_level = true;
  params.remove_standard_frame = true;
  window_->Init(params);

  // Add web view.
  SetLayoutManager(new views::FillLayout);
  set_background(views::Background::CreateStandardPanelBackground());
  web_view_->SetWebContents(web_contents);
  AddChildView(web_view_);

  int width = 800, height = 600;
  options.Get(switches::kWidth, &width);
  options.Get(switches::kHeight, &height);

  bool use_content_size;
  gfx::Rect bounds(0, 0, width, height);
  if (has_frame_ &&
      options.Get(switches::kUseContentSize, &use_content_size) &&
      use_content_size)
    bounds = window_->non_client_view()->GetWindowBoundsForClientBounds(bounds);

  window_->CenterWindow(bounds.size());
}

NativeWindowViews::~NativeWindowViews() {
}

void NativeWindowViews::Close() {
  window_->Close();
}

void NativeWindowViews::CloseImmediately() {
  window_->CloseNow();
}

void NativeWindowViews::Move(const gfx::Rect& bounds) {
  window_->SetBounds(bounds);
}

void NativeWindowViews::Focus(bool focus) {
  if (focus)
    window_->Activate();
  else
    window_->Deactivate();
}

bool NativeWindowViews::IsFocused() {
  return window_->IsActive();
}

void NativeWindowViews::Show() {
  window_->Show();
}

void NativeWindowViews::Hide() {
  window_->Hide();
}

bool NativeWindowViews::IsVisible() {
  return window_->IsVisible();
}

void NativeWindowViews::Maximize() {
  window_->Maximize();
}

void NativeWindowViews::Unmaximize() {
  window_->Restore();
}

void NativeWindowViews::Minimize() {
  window_->Minimize();
}

void NativeWindowViews::Restore() {
  window_->Restore();
}

void NativeWindowViews::SetFullscreen(bool fullscreen) {
  window_->SetFullscreen(fullscreen);
}

bool NativeWindowViews::IsFullscreen() {
  return window_->IsFullscreen();
}

void NativeWindowViews::SetSize(const gfx::Size& size) {
  window_->SetSize(size);
}

gfx::Size NativeWindowViews::GetSize() {
  return window_->GetWindowBoundsInScreen().size();
}

void NativeWindowViews::SetContentSize(const gfx::Size& size) {
  if (!has_frame_) {
    SetSize(size);
    return;
  }

  gfx::Rect bounds = window_->GetWindowBoundsInScreen();
  bounds.set_size(size);
  window_->SetBounds(
      window_->non_client_view()->GetWindowBoundsForClientBounds(bounds));
}

gfx::Size NativeWindowViews::GetContentSize() {
  if (!has_frame_)
    return GetSize();

  return window_->non_client_view()->frame_view()->
      GetBoundsForClientView().size();
}

void NativeWindowViews::SetMinimumSize(const gfx::Size& size) {
  minimum_size_ = size;
}

gfx::Size NativeWindowViews::GetMinimumSize() {
  return minimum_size_;
}

void NativeWindowViews::SetMaximumSize(const gfx::Size& size) {
  maximum_size_ = size;
}

gfx::Size NativeWindowViews::GetMaximumSize() {
  return maximum_size_;
}

void NativeWindowViews::SetResizable(bool resizable) {
  resizable_ = resizable;
}

bool NativeWindowViews::IsResizable() {
  return resizable_;
}

void NativeWindowViews::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
}

bool NativeWindowViews::IsAlwaysOnTop() {
  return window_->IsAlwaysOnTop();
}

void NativeWindowViews::Center() {
  window_->CenterWindow(GetSize());
}

void NativeWindowViews::SetPosition(const gfx::Point& position) {
  window_->SetBounds(gfx::Rect(position, GetSize()));
}

gfx::Point NativeWindowViews::GetPosition() {
  return window_->GetWindowBoundsInScreen().origin();
}

void NativeWindowViews::SetTitle(const std::string& title) {
  title_ = title;
  window_->UpdateWindowTitle();
}

std::string NativeWindowViews::GetTitle() {
  return title_;
}

void NativeWindowViews::FlashFrame(bool flash) {
  window_->FlashFrame(flash);
}

void NativeWindowViews::SetSkipTaskbar(bool skip) {
  // FIXME
}

void NativeWindowViews::SetKiosk(bool kiosk) {
  SetFullscreen(kiosk);
}

bool NativeWindowViews::IsKiosk() {
  return IsFullscreen();
}

gfx::NativeWindow NativeWindowViews::GetNativeWindow() {
  return window_->GetNativeWindow();
}

void NativeWindowViews::UpdateDraggableRegions(
    const std::vector<DraggableRegion>& regions) {
  // FIXME
}

void NativeWindowViews::DeleteDelegate() {
  NotifyWindowClosed();
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return web_view_;
}

bool NativeWindowViews::CanResize() const {
  return resizable_;
}

bool NativeWindowViews::CanMaximize() const {
  return resizable_;
}

base::string16 NativeWindowViews::GetWindowTitle() const {
  return base::UTF8ToUTF16(title_);
}

bool NativeWindowViews::ShouldHandleSystemCommands() const {
  return true;
}

gfx::ImageSkia NativeWindowViews::GetWindowAppIcon() {
  if (icon_)
    return *(icon_->ToImageSkia());
  else
    return gfx::ImageSkia();
}

gfx::ImageSkia NativeWindowViews::GetWindowIcon() {
  return GetWindowAppIcon();
}

views::Widget* NativeWindowViews::GetWidget() {
  return window_.get();
}

const views::Widget* NativeWindowViews::GetWidget() const {
  return window_.get();
}

views::View* NativeWindowViews::GetContentsView() {
  return this;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, this);
}

views::NonClientFrameView* NativeWindowViews::CreateNonClientFrameView(
    views::Widget* widget) {
  views::CustomFrameView* frame_view = new views::CustomFrameView;
  frame_view->Init(widget);
  return frame_view;
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   const mate::Dictionary& options) {
  return new NativeWindowViews(web_contents, options);
}

}  // namespace atom
