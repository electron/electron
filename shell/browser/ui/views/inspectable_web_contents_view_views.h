// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "chrome/browser/devtools/devtools_contents_resizing_strategy.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "ui/views/view.h"

namespace views {
class WebView;
class Widget;
class WidgetDelegate;
}  // namespace views

namespace electron {

class InspectableWebContents;

class InspectableWebContentsViewViews : public InspectableWebContentsView,
                                        public views::View {
 public:
  explicit InspectableWebContentsViewViews(
      InspectableWebContents* inspectable_web_contents);
  ~InspectableWebContentsViewViews() override;

  // InspectableWebContentsView:
  views::View* GetView() override;
  views::View* GetWebView() override;
  void ShowDevTools(bool activate) override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  bool IsDevToolsViewFocused() override;
  void SetIsDocked(bool docked, bool activate) override;
  void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;
  void SetTitle(const std::u16string& title) override;

  // views::View:
  void Layout() override;
  bool GetNeedsNotificationWhenVisibleBoundsChange() const override;
  void OnVisibleBoundsChanged() override;

  InspectableWebContents* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

  const std::u16string& GetTitle() const { return title_; }

 private:
  // Owns us.
  InspectableWebContents* inspectable_web_contents_;

  std::unique_ptr<views::Widget> devtools_window_;
  views::WebView* devtools_window_web_view_ = nullptr;
  views::View* contents_web_view_ = nullptr;
  views::WebView* devtools_web_view_ = nullptr;

  DevToolsContentsResizingStrategy strategy_;
  gfx::Rect visible_bounds_;
  bool devtools_visible_ = false;
  views::WidgetDelegate* devtools_window_delegate_ = nullptr;
  std::u16string title_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
