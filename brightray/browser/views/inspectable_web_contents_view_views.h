#ifndef BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
#define BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_

#include "browser/devtools_contents_resizing_strategy.h"
#include "browser/inspectable_web_contents_view.h"

#include "base/compiler_specific.h"
#include "ui/views/view.h"

namespace views {
class WebView;
class Widget;
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
  virtual views::View* GetView() override;
  virtual views::View* GetWebView() override;
  virtual void ShowDevTools() override;
  virtual void CloseDevTools() override;
  virtual bool IsDevToolsViewShowing() override;
  virtual void SetIsDocked(bool docked) override;
  virtual void SetContentsResizingStrategy(
      const DevToolsContentsResizingStrategy& strategy) override;

  InspectableWebContentsImpl* inspectable_web_contents() {
    return inspectable_web_contents_;
  }

 private:
  // views::View:
  virtual void Layout() override;

  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  scoped_ptr<views::Widget> devtools_window_;
  views::WebView* devtools_window_web_view_;
  views::WebView* contents_web_view_;
  views::WebView* devtools_web_view_;

  DevToolsContentsResizingStrategy strategy_;
  bool devtools_visible_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewViews);
};

}  // namespace brightray

#endif  // BROWSER_VIEWS_INSPECTABLE_WEB_CONTENTS_VIEW_VIEWS_H_
