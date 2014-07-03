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
#include "ui/views/window/native_frame_view.h"
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
  params.top_level = true;
  params.remove_standard_frame = !has_frame_;
  window_->Init(params);

  int width = 800, height = 600;
  options.Get(switches::kWidth, &width);
  options.Get(switches::kHeight, &height);
  window_->CenterWindow(gfx::Size(width, height));

  SetLayoutManager(new views::FillLayout);
  set_background(views::Background::CreateStandardPanelBackground());

  web_view_->SetWebContents(web_contents);
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
  // FIXME
  SetSize(size);
}

gfx::Size NativeWindowViews::GetContentSize() {
  // FIXME
  return GetSize();
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
  // FIXME
  resizable_ = resizable;
}

bool NativeWindowViews::IsResizable() {
  return resizable_;
}

void NativeWindowViews::SetAlwaysOnTop(bool top) {
  window_->SetAlwaysOnTop(top);
}

bool NativeWindowViews::IsAlwaysOnTop() {
  // FIXME
  return false;
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
}

void NativeWindowViews::ViewHierarchyChanged(
  const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this) {
    AddChildView(web_view_);
  }
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

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   const mate::Dictionary& options) {
  return new NativeWindowViews(web_contents, options);
}

}  // namespace atom
