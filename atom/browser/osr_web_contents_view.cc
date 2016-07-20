// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/osr_web_contents_view.h"

#include "atom/browser/osr_window.h"
#include <iostream>

namespace atom {

OffScreenWebContentsView::OffScreenWebContentsView() {
  std::cout << "OffScreenWebContentsView" << std::endl;
  //std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}
OffScreenWebContentsView::~OffScreenWebContentsView() {
  std::cout << "~OffScreenWebContentsView" << std::endl;
}

// Returns the native widget that contains the contents of the tab.
gfx::NativeView OffScreenWebContentsView::GetNativeView() const{
  std::cout << "GetNativeView" << std::endl;
  return gfx::NativeView();
}

// Returns the native widget with the main content of the tab (i.e. the main
// render view host, though there may be many popups in the tab as children of
// the container).
gfx::NativeView OffScreenWebContentsView::GetContentNativeView() const{
  std::cout << "GetContentNativeView" << std::endl;
  return gfx::NativeView();
}

// Returns the outermost native view. This will be used as the parent for
// dialog boxes.
gfx::NativeWindow OffScreenWebContentsView::GetTopLevelNativeWindow() const{
  std::cout << "GetTopLevelNativeWindow" << std::endl;
  return gfx::NativeWindow();
}

// Computes the rectangle for the native widget that contains the contents of
// the tab in the screen coordinate system.
void OffScreenWebContentsView::GetContainerBounds(gfx::Rect* out) const{
  std::cout << "GetContainerBounds" << std::endl;
  *out = GetViewBounds();
}

// TODO(brettw) this is a hack. It's used in two places at the time of this
// writing: (1) when render view hosts switch, we need to size the replaced
// one to be correct, since it wouldn't have known about sizes that happened
// while it was hidden; (2) in constrained windows.
//
// (1) will be fixed once interstitials are cleaned up. (2) seems like it
// should be cleaned up or done some other way, since this works for normal
// WebContents without the special code.
void OffScreenWebContentsView::SizeContents(const gfx::Size& size){
  std::cout << "SizeContents" << std::endl;
}

// Sets focus to the native widget for this tab.
void OffScreenWebContentsView::Focus(){
  std::cout << "OffScreenWebContentsView::Focus" << std::endl;
}

// Sets focus to the appropriate element when the WebContents is shown the
// first time.
void OffScreenWebContentsView::SetInitialFocus(){
  std::cout << "SetInitialFocus" << std::endl;
}

// Stores the currently focused view.
void OffScreenWebContentsView::StoreFocus(){
  std::cout << "StoreFocus" << std::endl;
}

// Restores focus to the last focus view. If StoreFocus has not yet been
// invoked, SetInitialFocus is invoked.
void OffScreenWebContentsView::RestoreFocus(){
  std::cout << "RestoreFocus" << std::endl;
}

// Returns the current drop data, if any.
content::DropData* OffScreenWebContentsView::GetDropData() const{
  std::cout << "GetDropData" << std::endl;
  return nullptr;
}

// Get the bounds of the View, relative to the parent.
gfx::Rect OffScreenWebContentsView::GetViewBounds() const{
  std::cout << "OffScreenWebContentsView::GetViewBounds" << std::endl;
  return view_ ? view_->GetViewBounds() : gfx::Rect();
}

void OffScreenWebContentsView::CreateView(
    const gfx::Size& initial_size, gfx::NativeView context){
  std::cout << "CreateView" << std::endl;
  std::cout << initial_size.width() << "x" << initial_size.height() << std::endl;
}

// Sets up the View that holds the rendered web page, receives messages for
// it and contains page plugins. The host view should be sized to the current
// size of the WebContents.
//
// |is_guest_view_hack| is temporary hack and will be removed once
// RenderWidgetHostViewGuest is not dependent on platform view.
// TODO(lazyboy): Remove |is_guest_view_hack| once http://crbug.com/330264 is
// fixed.
content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForWidget(
    content::RenderWidgetHost* render_widget_host, bool is_guest_view_hack){
  std::cout << "CreateViewForWidget" << std::endl;
  view_ = new OffScreenWindow(render_widget_host);
  return view_;
}

// Creates a new View that holds a popup and receives messages for it.
content::RenderWidgetHostViewBase*
  OffScreenWebContentsView::CreateViewForPopupWidget(
    content::RenderWidgetHost* render_widget_host){
  std::cout << "CreateViewForPopupWidget" << std::endl;
  view_ = new OffScreenWindow(render_widget_host);
  return view_;
}

// Sets the page title for the native widgets corresponding to the view. This
// is not strictly necessary and isn't expected to be displayed anywhere, but
// can aid certain debugging tools such as Spy++ on Windows where you are
// trying to find a specific window.
void OffScreenWebContentsView::SetPageTitle(const base::string16& title){
  std::cout << "SetPageTitle" << std::endl;
  std::cout << title << std::endl;
}

// Invoked when the WebContents is notified that the RenderView has been
// fully created.
void OffScreenWebContentsView::RenderViewCreated(content::RenderViewHost* host){
  std::cout << "RenderViewCreated" << std::endl;
}

// Invoked when the WebContents is notified that the RenderView has been
// swapped in.
void OffScreenWebContentsView::RenderViewSwappedIn(content::RenderViewHost* host){
  std::cout << "RenderViewSwappedIn" << std::endl;
}

// Invoked to enable/disable overscroll gesture navigation.
void OffScreenWebContentsView::SetOverscrollControllerEnabled(bool enabled){
  std::cout << "SetOverscrollControllerEnabled" << std::endl;
}

#if defined(OS_MACOSX)
void OffScreenWebContentsView::SetAllowOtherViews(bool allow) {
}

bool OffScreenWebContentsView::GetAllowOtherViews() const {
  return false;
}

bool OffScreenWebContentsView::IsEventTracking() const {
  return false;
}

void OffScreenWebContentsView::CloseTabAfterEventTracking() {
}
#endif  // defined(OS_MACOSX)

}  // namespace atom
