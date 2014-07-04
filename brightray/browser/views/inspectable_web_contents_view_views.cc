#include "browser/views/inspectable_web_contents_view_views.h"

#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/views/controls/webview/webview.h"

namespace brightray {

InspectableWebContentsView* CreateInspectableContentsView(
    InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewViews(inspectable_web_contents);
}

InspectableWebContentsViewViews::InspectableWebContentsViewViews(
    InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents),
      contents_web_view_(new views::WebView(NULL)),
      devtools_web_view_(new views::WebView(NULL)) {
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
  if (devtools_web_view_->visible())
    return;

  devtools_web_view_->SetVisible(true);
  devtools_web_view_->SetWebContents(inspectable_web_contents_->devtools_web_contents());
  Layout();
}

void InspectableWebContentsViewViews::CloseDevTools() {
  if (!devtools_web_view_->visible())
    return;

  devtools_web_view_->SetVisible(false);
  devtools_web_view_->SetWebContents(NULL);
  Layout();
}

bool InspectableWebContentsViewViews::IsDevToolsViewShowing() {
  return devtools_web_view_->visible();
}

void InspectableWebContentsViewViews::SetIsDocked(bool docked) {
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
