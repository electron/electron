#ifndef BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
#define BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_

#include "brightray/browser/inspectable_web_contents_view.h"

#include "base/mac/scoped_nsobject.h"

@class BRYInspectableWebContentsView;

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewMac : public InspectableWebContentsView {
 public:
  explicit InspectableWebContentsViewMac(
      InspectableWebContentsImpl* inspectable_web_contents_impl);
  ~InspectableWebContentsViewMac() override;

  gfx::NativeView GetNativeView() const override;
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

 private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  base::scoped_nsobject<BRYInspectableWebContentsView> view_;

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewMac);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_INSPECTABLE_WEB_CONTENTS_VIEW_MAC_H_
