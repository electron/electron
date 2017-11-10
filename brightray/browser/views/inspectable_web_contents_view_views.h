#ifndef BRIGHTRAY_BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
#define BRIGHTRAY_BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_

#include "base/compiler_specific.h"
#include "brightray/browser/devtools_contents_resizing_strategy.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "ui/views/view.h"

namespace views {
class WebView;
class Widget;
class WidgetDelegate;
}

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewViews : public InspectableWebContentsView,
                                        public views::View {
 public:
  explicit InspectableWebContentsViewViews(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  ~InspectableWebContentsViewViews();

  // InspectableWebContentsView:
  views::View* GetView() override;
  views::View* GetWebView() override;
  void ShowDevTools() override;
  void CloseDevTools() override;
  bool IsDevToolsViewShowing() override;
  bool IsDevToolsViewFocused() override;
  void SetIsDocked(bool docked) override;
  void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;
  void SetTitle(const base::string16& title) override;

  InspectableWebContentsImpl* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

  const base::string16& GetTitle() const { return title_; }

 private:
  // views::View:
  void Layout() override;

  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  std::unique_ptr<views::Widget> devtools_window_;
  views::WebView* devtools_window_web_view_;
  views::View* contents_web_view_;
  views::WebView* devtools_web_view_;

  DevToolsContentsResizingStrategy strategy_;
  bool devtools_visible_;
  views::WidgetDelegate* devtools_window_delegate_;
  base::string16 title_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewViews);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
