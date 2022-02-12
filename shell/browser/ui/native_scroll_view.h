// Copyright (c) 2022 GitHub, Inc.

#ifndef ELECTRON_SHELL_BROWSER_UI_NATIVE_SCROLL_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_NATIVE_SCROLL_VIEW_H_

#include "shell/browser/ui/native_view.h"

namespace electron {

enum class ScrollBarMode { kDisabled, kHiddenButEnabled, kEnabled };

#if BUILDFLAG(IS_MAC)
enum class ScrollElasticity {
  kAutomatic = 0,  // NSScrollElasticityAutomatic = 0
  kNone = 1,       // NSScrollElasticityNone = 1
  kAllowed = 2,    // NSScrollElasticityAllowed = 2
};
#endif

class NativeScrollView : public NativeView {
 public:
  NativeScrollView();

  // NativeView:
  void DetachChildView(NativeView* view) override;
  void TriggerBeforeunloadEvents() override;
#if BUILDFLAG(IS_MAC)
  void UpdateDraggableRegions() override;
#endif

  void SetContentView(scoped_refptr<NativeView> view);
  NativeView* GetContentView() const;

  void SetHorizontalScrollBarMode(ScrollBarMode mode);
  ScrollBarMode GetHorizontalScrollBarMode() const;
  void SetVerticalScrollBarMode(ScrollBarMode mode);
  ScrollBarMode GetVerticalScrollBarMode() const;

#if BUILDFLAG(IS_MAC)
  void SetHorizontalScrollElasticity(ScrollElasticity elasticity);
  ScrollElasticity GetHorizontalScrollElasticity() const;
  void SetVerticalScrollElasticity(ScrollElasticity elasticity);
  ScrollElasticity GetVerticalScrollElasticity() const;
#endif

 protected:
  ~NativeScrollView() override;

  // NativeView:
  void SetWindowForChildren(NativeWindow* window) override;

  void InitScrollView();
  void SetContentViewImpl(NativeView* container);
  void DetachChildViewImpl();

 private:
  scoped_refptr<NativeView> content_view_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_NATIVE_SCROLL_VIEW_H_
