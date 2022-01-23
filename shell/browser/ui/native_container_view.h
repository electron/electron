#ifndef SHELL_BROWSER_UI_NATIVE_CONTAINER_VIEW_H_
#define SHELL_BROWSER_UI_NATIVE_CONTAINER_VIEW_H_

#include <vector>

#include "shell/browser/ui/native_view.h"

namespace electron {

class NativeContainerView : public NativeView {
 public:
  NativeContainerView();

  // NativeView:
  bool IsContainer() const override;
  void DetachChildView(NativeView* view) override;
  void TriggerBeforeunloadEvents() override;
#if defined(OS_MAC)
  void UpdateDraggableRegions() override;
#endif

  // Add/Remove children.
  void AddChildView(scoped_refptr<NativeView> view);
  bool RemoveChildView(NativeView* view);

  // Get children.
  int ChildCount() const { return static_cast<int>(children_.size()); }

 protected:
  ~NativeContainerView() override;

  // NativeView:
  void SetWindowForChildren(NativeWindow* window) override;

  void InitContainerView();
  void AddChildViewImpl(NativeView* view);
  void RemoveChildViewImpl(NativeView* view);

 private:
  // The view layer.
  std::vector<scoped_refptr<NativeView>> children_;
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_NATIVE_CONTAINER_VIEW_H_
