// Copyright (c) 2022 GitHub, Inc.

#ifndef ELECTRON_SHELL_BROWSER_UI_NATIVE_VIEW_H_
#define ELECTRON_SHELL_BROWSER_UI_NATIVE_VIEW_H_

#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/supports_user_data.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
#include "ui/views/view_observer.h"
#endif

#if BUILDFLAG(IS_MAC)
#ifdef __OBJC__
@class NSView;
#else
struct NSView;
#endif
#elif defined(TOOLKIT_VIEWS)
namespace views {
class View;
}
#endif

namespace electron {

#if BUILDFLAG(IS_MAC)
using NATIVEVIEW = NSView*;
#elif defined(TOOLKIT_VIEWS)
using NATIVEVIEW = views::View*;
#endif

class NativeWindow;

// The base class for all kinds of views.
class NativeView : public base::RefCounted<NativeView>,
                   public base::SupportsUserData
#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
    ,
                   public views::ViewObserver
#endif
{
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnChildViewDetached(NativeView* observed_view,
                                     NativeView* view) {}
    virtual void OnViewIsDeleting(NativeView* observed_view) {}
  };

  NativeView();

  // Change position and size.
  virtual void SetBounds(const gfx::Rect& bounds);

  // Get position and size.
  gfx::Rect GetBounds() const;

  // Display related styles.
  void SetBackgroundColor(SkColor color);

#if BUILDFLAG(IS_MAC)
  void SetWantsLayer(bool wants);
  bool WantsLayer() const;
#endif

  // Get parent.
  NativeView* GetParent() const { return parent_; }

  // Get window.
  NativeWindow* GetWindow() const { return window_; }

  // Get the native View object.
  NATIVEVIEW GetNative() const { return view_; }

  // Internal: Set parent view.
  void SetParent(NativeView* parent);
  void BecomeContentView(NativeWindow* window);

  void SetWindow(NativeWindow* window);

  // Whether this class inherits from Container.
  virtual bool IsContainer() const;

  virtual void DetachChildView(NativeView* view);

  virtual void TriggerBeforeunloadEvents();

#if BUILDFLAG(IS_MAC)
  virtual void UpdateDraggableRegions();
#endif

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
  // Should delete the |view_| in destructor.
  void set_delete_view(bool should) { delete_view_ = should; }
#endif

  void NotifyChildViewDetached(NativeView* view);

  // Notify that native view is destroyed.
  void NotifyViewIsDeleting();

 protected:
  ~NativeView() override;

  void SetNativeView(NATIVEVIEW view);

  void InitView();
  void DestroyView();

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
  // views::ViewObserver:
  void OnViewIsDeleting(views::View* observed_view) override;
#endif

  virtual void SetWindowForChildren(NativeWindow* window);

 private:
  friend class base::RefCounted<NativeView>;

  base::ObserverList<Observer> observers_;

  // Relationships.
  NativeView* parent_ = nullptr;
  NativeWindow* window_ = nullptr;

  // The native implementation.
  NATIVEVIEW view_ = nullptr;

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
  bool delete_view_ = true;
#endif
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UI_NATIVE_VIEW_H_
