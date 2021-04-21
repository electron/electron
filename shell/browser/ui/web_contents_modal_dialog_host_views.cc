#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "shell/browser/native_window.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/client_view.h"

using views::View;
using web_modal::ModalDialogHostObserver;
using web_modal::WebContentsModalDialogHost;

namespace electron {
class WebContentsModalDialogHostViews : public WebContentsModalDialogHost {
 public:
  explicit WebContentsModalDialogHostViews(NativeWindow* native_window)
      : native_window_(native_window) {}

  ~WebContentsModalDialogHostViews() override {
    for (ModalDialogHostObserver& observer : observer_list_)
      observer.OnHostDestroying();
  }

  void NotifyPositionRequiresUpdate() {
    for (ModalDialogHostObserver& observer : observer_list_)
      observer.OnPositionRequiresUpdate();
  }

  gfx::Point GetDialogPosition(const gfx::Size& size) override {
    views::View* view = native_window_->content_view();
    gfx::Rect content_area = view->ConvertRectToWidget(view->GetLocalBounds());
    const int middle_x = content_area.x() + content_area.width() / 2;
    const int top = margin_top_;
    return gfx::Point(middle_x - size.width() / 2, top);
  }

  bool ShouldActivateDialog() const override {
    // The browser Widget may be inactive if showing the dialog so instead check
    // if the window is active before activating the dialog.
    return native_window_->IsFocused();
  }

  gfx::Size GetMaximumDialogSize() override {
    views::View* view = native_window_->content_view();
    gfx::Rect content_area = view->ConvertRectToWidget(view->GetLocalBounds());
    const int top = margin_top_;
    return gfx::Size(content_area.width(), content_area.bottom() - top);
  }

  void SetMarginTop(int top) { margin_top_ = top; }

 private:
  gfx::NativeView GetHostView() const override {
    return native_window_->GetNativeView();
  }

  // Add/remove observer.
  void AddObserver(ModalDialogHostObserver* observer) override {
    observer_list_.AddObserver(observer);
  }
  void RemoveObserver(ModalDialogHostObserver* observer) override {
    observer_list_.RemoveObserver(observer);
  }

  NativeWindow* native_window_;

  int margin_top_ = 0;

  base::ObserverList<ModalDialogHostObserver>::Unchecked observer_list_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsModalDialogHostViews);
};
}  // namespace electron
