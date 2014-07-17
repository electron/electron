// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/native_window_views.h"

#include <string>
#include <vector>

#include "atom/browser/ui/views/menu_bar.h"
#include "atom/browser/ui/views/menu_layout.h"
#include "atom/common/draggable_region.h"
#include "atom/common/options_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/inspectable_web_contents_view.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents_view.h"
#include "native_mate/dictionary.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/background.h"
#include "ui/views/controls/webview/unhandled_keyboard_event_handler.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/window/client_view.h"
#include "ui/views/widget/widget.h"

#if defined(USE_X11)
#include "atom/browser/ui/views/global_menu_bar_x11.h"
#include "atom/browser/ui/views/linux_frame_view.h"
#elif defined(OS_WIN)
#include "atom/browser/ui/views/win_frame_view.h"
#endif

namespace atom {

namespace {

// The menu bar height in pixels.
const int kMenuBarHeight = 25;

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
      menu_bar_(NULL),
      web_view_(inspectable_web_contents()->GetView()->GetView()),
      keyboard_event_handler_(new views::UnhandledKeyboardEventHandler),
      resizable_(true) {
  options.Get(switches::kResizable, &resizable_);
  options.Get(switches::kTitle, &title_);

  int width = 800, height = 600;
  options.Get(switches::kWidth, &width);
  options.Get(switches::kHeight, &height);
  gfx::Rect bounds(0, 0, width, height);

  window_->AddObserver(this);

  views::Widget::InitParams params;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.bounds = bounds;
  params.delegate = this;
  params.type = views::Widget::InitParams::TYPE_WINDOW;
  params.top_level = true;

#if defined(USE_X11)
  // In X11 the window frame is drawn by the application.
  params.remove_standard_frame = true;
#elif defined(OS_WIN)
  if (!has_frame_)
    params.remove_standard_frame = true;
#endif

  bool skip_taskbar = false;
  if (options.Get(switches::kSkipTaskbar, &skip_taskbar) && skip_taskbar)
    params.type = views::Widget::InitParams::TYPE_BUBBLE;

  window_->Init(params);

  // Add web view.
  SetLayoutManager(new MenuLayout(kMenuBarHeight));
  set_background(views::Background::CreateStandardPanelBackground());
  AddChildView(web_view_);

  bool use_content_size;
  if (has_frame_ &&
      options.Get(switches::kUseContentSize, &use_content_size) &&
      use_content_size)
    bounds = ContentBoundsToWindowBounds(bounds);

  window_->CenterWindow(bounds.size());
  Layout();
}

NativeWindowViews::~NativeWindowViews() {
  window_->RemoveObserver(this);
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

bool NativeWindowViews::IsMaximized() {
  return window_->IsMaximized();
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
  window_->SetBounds(ContentBoundsToWindowBounds(bounds));
}

gfx::Size NativeWindowViews::GetContentSize() {
  if (!has_frame_)
    return GetSize();

  gfx::Size content_size =
      window_->non_client_view()->frame_view()->GetBoundsForClientView().size();
  if (menu_bar_)
    content_size.set_height(content_size.height() - kMenuBarHeight);
  return content_size;
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
#if defined(OS_WIN)
  // FIXME
#endif
}

void NativeWindowViews::SetKiosk(bool kiosk) {
  SetFullscreen(kiosk);
}

bool NativeWindowViews::IsKiosk() {
  return IsFullscreen();
}

void NativeWindowViews::SetMenu(ui::MenuModel* menu_model) {
  RegisterAccelerators(menu_model);

#if defined(USE_X11)
  if (!global_menu_bar_)
    global_menu_bar_.reset(new GlobalMenuBarX11(this));

  // Use global application menu bar when possible.
  if (global_menu_bar_->IsServerStarted()) {
    global_menu_bar_->SetMenu(menu_model);
    return;
  }
#endif

  // Do not show menu bar in frameless window.
  if (!has_frame_)
    return;

  if (!menu_bar_) {
    gfx::Size content_size = GetContentSize();
    menu_bar_ = new MenuBar;
    AddChildViewAt(menu_bar_, 0);
    SetContentSize(content_size);
  }

  menu_bar_->SetMenu(menu_model);
  Layout();
}

gfx::NativeWindow NativeWindowViews::GetNativeWindow() {
  return window_->GetNativeWindow();
}

void NativeWindowViews::UpdateDraggableRegions(
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
}

void NativeWindowViews::OnWidgetActivationChanged(
    views::Widget* widget, bool active) {
  if (widget != window_.get())
    return;

  if (active)
    NotifyWindowFocus();
  else
    NotifyWindowBlur();
}

void NativeWindowViews::DeleteDelegate() {
  NotifyWindowClosed();
}

views::View* NativeWindowViews::GetInitiallyFocusedView() {
  return inspectable_web_contents()->GetView()->GetWebView();
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

bool NativeWindowViews::ShouldDescendIntoChildForEventHandling(
    gfx::NativeView child,
    const gfx::Point& location) {
  // App window should claim mouse events that fall within the draggable region.
  if (draggable_region_ &&
      draggable_region_->contains(location.x(), location.y()))
    return false;

  // And the events on border for dragging resizable frameless window.
  if (!has_frame_ && CanResize()) {
    FramelessView* frame = static_cast<FramelessView*>(
        window_->non_client_view()->frame_view());
    return frame->ResizingBorderHitTest(location) == HTNOWHERE;
  }

  return true;
}

views::ClientView* NativeWindowViews::CreateClientView(views::Widget* widget) {
  return new NativeWindowClientView(widget, this);
}

views::NonClientFrameView* NativeWindowViews::CreateNonClientFrameView(
    views::Widget* widget) {
#if defined(USE_X11)
  LinuxFrameView* frame_view =  new LinuxFrameView;
#else
  WinFrameView* frame_view =  new WinFrameView;
#endif
  frame_view->Init(this, widget);
  return frame_view;
}

void NativeWindowViews::HandleKeyboardEvent(
    content::WebContents*,
    const content::NativeWebKeyboardEvent& event) {
  keyboard_event_handler_->HandleKeyboardEvent(event, GetFocusManager());
}

bool NativeWindowViews::AcceleratorPressed(const ui::Accelerator& accelerator) {
  return accelerator_util::TriggerAcceleratorTableCommand(
      &accelerator_table_, accelerator);
}

void NativeWindowViews::RegisterAccelerators(ui::MenuModel* menu_model) {
  // Clear previous accelerators.
  views::FocusManager* focus_manager = GetFocusManager();
  accelerator_table_.clear();
  focus_manager->UnregisterAccelerators(this);

  // Register accelerators with focus manager.
  accelerator_util::GenerateAcceleratorTable(&accelerator_table_, menu_model);
  accelerator_util::AcceleratorTable::const_iterator iter;
  for (iter = accelerator_table_.begin();
       iter != accelerator_table_.end();
       ++iter) {
    focus_manager->RegisterAccelerator(
        iter->first, ui::AcceleratorManager::kNormalPriority, this);
  }
}

gfx::Rect NativeWindowViews::ContentBoundsToWindowBounds(
    const gfx::Rect& bounds) {
  gfx::Rect window_bounds =
      window_->non_client_view()->GetWindowBoundsForClientBounds(bounds);
  if (menu_bar_)
    window_bounds.set_height(window_bounds.height() + kMenuBarHeight);
  return window_bounds;
}

// static
NativeWindow* NativeWindow::Create(content::WebContents* web_contents,
                                   const mate::Dictionary& options) {
  return new NativeWindowViews(web_contents, options);
}

}  // namespace atom
