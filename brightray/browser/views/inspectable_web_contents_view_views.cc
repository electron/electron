#include "browser/views/inspectable_web_contents_view_views.h"

#include "browser/inspectable_web_contents_impl.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents_view.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/window/client_view.h"

namespace brightray {

namespace {

class DevToolsWindowDelegate : public views::ClientView,
                               public views::WidgetDelegate {
 public:
  DevToolsWindowDelegate(InspectableWebContentsViewViews* shell,
                         views::View* view,
                         views::Widget* widget)
      : views::ClientView(widget_, view),
        shell_(shell),
        view_(view),
        widget_(widget),
        title_(base::ASCIIToUTF16("Developer Tools")) {}
  virtual ~DevToolsWindowDelegate() {}

  // views::WidgetDelegate:
  virtual views::View* GetInitiallyFocusedView() OVERRIDE { return view_; }
  virtual bool CanResize() const OVERRIDE { return true; }
  virtual bool CanMaximize() const OVERRIDE { return true; }
  virtual base::string16 GetWindowTitle() const OVERRIDE { return title_; }
  virtual views::Widget* GetWidget() OVERRIDE { return widget_; }
  virtual const views::Widget* GetWidget() const OVERRIDE { return widget_; }
  virtual views::View* GetContentsView() OVERRIDE { return view_; }
  virtual views::ClientView* CreateClientView(views::Widget* widget) { return this; }

  // views::ClientView:
  virtual bool CanClose() OVERRIDE {
    shell_->inspectable_web_contents()->CloseDevTools();
    return false;
  }

 private:
  InspectableWebContentsViewViews* shell_;
  views::View* view_;
  views::Widget* widget_;
  base::string16 title_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsWindowDelegate);
};

}  // namespace

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewViews(inspectable_web_contents);
}

InspectableWebContentsViewViews::InspectableWebContentsViewViews(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      devtools_window_web_view_(NULL),
      contents_web_view_(new views::WebView(NULL)),
      devtools_web_view_(new views::WebView(NULL)),
      devtools_visible_(false) {
  set_owned_by_client();

  devtools_web_view_->SetVisible(false);
  contents_web_view_->SetWebContents(inspectable_web_contents_->GetWebContents());
  AddChildView(devtools_web_view_);
  AddChildView(contents_web_view_);
}

InspectableWebContentsViewViews::~InspectableWebContentsViewViews() {
}

views::View* InspectableWebContentsViewViews::GetView() {
  return this;
}

views::View* InspectableWebContentsViewViews::GetWebView() {
  return contents_web_view_;
}

gfx::NativeView InspectableWebContentsViewViews::GetNativeView() const {
  return inspectable_web_contents_->GetWebContents()->GetView()->GetNativeView();
}

void InspectableWebContentsViewViews::ShowDevTools() {
  if (devtools_visible_)
    return;

  devtools_visible_ = true;
  if (devtools_window_) {
    devtools_window_web_view_->SetWebContents(inspectable_web_contents_->devtools_web_contents());
    devtools_window_->CenterWindow(gfx::Size(800, 600));
    devtools_window_->Show();
  } else {
    devtools_web_view_->SetVisible(true);
    devtools_web_view_->SetWebContents(inspectable_web_contents_->devtools_web_contents());
    devtools_web_view_->RequestFocus();
    Layout();
  }
}

void InspectableWebContentsViewViews::CloseDevTools() {
  if (!devtools_visible_)
    return;

  devtools_visible_ = false;
  if (devtools_window_) {
    devtools_window_.reset();
    devtools_window_web_view_ = NULL;
  } else {
    devtools_web_view_->SetVisible(false);
    devtools_web_view_->SetWebContents(NULL);
    Layout();
  }
}

bool InspectableWebContentsViewViews::IsDevToolsViewShowing() {
  return devtools_visible_;
}

void InspectableWebContentsViewViews::SetIsDocked(bool docked) {
  CloseDevTools();

  if (!docked) {
    devtools_window_.reset(new views::Widget);
    devtools_window_web_view_ = new views::WebView(NULL);

    views::Widget::InitParams params;
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    // The delegate is also a ClientView, so it would deleted when the view
    // destructs, no need to delete it in DeleteDelegate.
    params.delegate = new DevToolsWindowDelegate(this,
                                                 devtools_window_web_view_,
                                                 devtools_window_.get());
    params.top_level = true;
    params.remove_standard_frame = true;
    devtools_window_->Init(params);
  }

  ShowDevTools();
}

void InspectableWebContentsViewViews::SetContentsResizingStrategy(
    const DevToolsContentsResizingStrategy& strategy) {
  strategy_.CopyFrom(strategy);
  Layout();
}

void InspectableWebContentsViewViews::Layout() {
  if (!devtools_web_view_->visible()) {
    contents_web_view_->SetBoundsRect(GetContentsBounds());
    return;
  }

  gfx::Size container_size(width(), height());
  gfx::Rect old_devtools_bounds(devtools_web_view_->bounds());
  gfx::Rect old_contents_bounds(contents_web_view_->bounds());
  gfx::Rect new_devtools_bounds;
  gfx::Rect new_contents_bounds;
  ApplyDevToolsContentsResizingStrategy(strategy_, container_size,
      old_devtools_bounds, old_contents_bounds,
      &new_devtools_bounds, &new_contents_bounds);

  // DevTools cares about the specific position, so we have to compensate RTL
  // layout here.
  new_devtools_bounds.set_x(GetMirroredXForRect(new_devtools_bounds));
  new_contents_bounds.set_x(GetMirroredXForRect(new_contents_bounds));

  devtools_web_view_->SetBoundsRect(new_devtools_bounds);
  contents_web_view_->SetBoundsRect(new_contents_bounds);
}

}  // namespace brightray
