// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_

#include <memory>
#include <optional>

#include "base/memory/weak_ptr.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/native_peer.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_helper/constructible.h"
#include "ui/views/layout/proposed_layout.h"
#include "ui/views/view_observer.h"
#include "v8/include/cppgc/garbage-collected.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/v8-traced-handle.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace gin {
class Arguments;
}  // namespace gin

namespace gin_helper {
class ErrorThrower;
}  // namespace gin_helper

namespace ui {
class Layer;
}  // namespace ui

namespace views {
class SizeBounds;
class View;
}  // namespace views

namespace electron::api {

// The JavaScript View. The cppgc managed wrapper holds the API state and the
// GC edges to its child views. Its native peer, View::Host, owns the
// views::View and holds every native registration, so that nothing native is
// torn down while cppgc sweeps the wrapper.
//
// ImageView and WebContentsView are C++ subclasses. They share View's pointer
// tag and have their own WrapperInfo, so gin::Converter<View*> accepts all
// three while their own converters accept only their exact type.
class View : public gin::Wrappable<View>,
             public gin_helper::EventEmitterMixin<View>,
             public gin_helper::Constructible<View> {
 public:
  class Host;

  static View* New(gin::Arguments* args);

  // Creates a View through its JavaScript constructor, so its wrapper has the
  // right prototype even if `electron.View` has never been loaded.
  static View* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  // gin_helper::Constructible
  static void FillObjectTemplate(v8::Isolate* isolate,
                                 v8::Local<v8::ObjectTemplate> templ);
  static const char* GetClassName() { return "View"; }

  // Make public for cppgc::MakeGarbageCollected.
  View();
  ~View() override;

  // disable copy
  View(const View&) = delete;
  View& operator=(const View&) = delete;

  void AddChildViewAt(gin_helper::ErrorThrower thrower,
                      View* child,
                      std::optional<size_t> index);
  void RemoveChildView(View* child);

  void SetBounds(const gfx::Rect& bounds, gin::Arguments* args);
  gfx::Rect GetBounds() const;
  void SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value);
  v8::Local<v8::Value> GetChildren(v8::Isolate* isolate);
  void SetBackgroundColor(std::optional<WrappedSkColor> color);
  void SetBorderRadius(int radius);
  void SetBackgroundBlur(int blur_radius);
  virtual void SetInteractive(bool interactive);
  bool GetInteractive() const { return interactive_; }
  void SetVisible(bool visible);
  bool GetVisible() const;

  views::View* view() const;
  std::optional<int> border_radius() const { return border_radius_; }

 protected:
  explicit View(std::unique_ptr<views::View> view);

  struct DeferHost {};
  explicit View(DeferHost);
  void SetHost(std::unique_ptr<Host, NativePeerBase::Deleter> host);
  Host* host() const { return host_.get(); }

  virtual bool IsUsable() const;

 private:
  friend class Host;

  // A node in the singly linked list of child views, in z-order.
  class ChildEntry final : public cppgc::GarbageCollected<ChildEntry> {
   public:
    ChildEntry(View* view, ChildEntry* next);
    ~ChildEntry();
    void Trace(cppgc::Visitor* visitor) const;

    cppgc::Member<View> view;
    cppgc::Member<ChildEntry> next;
  };

  views::View* live_view() const;
  void OnBoundsChanged();
  void OnChildViewRemoved(views::View* child);
  views::ProposedLayout CalculateProposedLayout(
      v8::Isolate* isolate,
      const views::SizeBounds& size_bounds);

  size_t GetChildCount() const;
  void InsertChild(View* child, size_t index);
  void InsertChildEntry(ChildEntry* entry, size_t index);
  // Unlinks and returns the entry whose View has |child_view| as its native
  // view, or null. Entries are matched by native view because that is what
  // views::View reports when it reparents or removes a child.
  ChildEntry* TakeChildEntry(views::View* child_view);

  ui::Layer* GetLayer();
  void ApplyBorderRadius();

  cppgc::Member<ChildEntry> first_child_;
  v8::TracedReference<v8::Function> layout_callback_;
  std::optional<int> border_radius_;
  std::unique_ptr<Host, NativePeerBase::Deleter> host_;
};

class View::Host : public NativePeer<View>, public views::ViewObserver {
 public:
  Host(View* wrapper, std::unique_ptr<views::View> view);

  // Non-null until the peer is released. The view is owned by client, so no
  // parent deletes it.
  views::View* view() const { return view_.get(); }

  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds);

  base::WeakPtr<Host> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

 protected:
  ~Host() override;

  // Marks |view| as owned by whatever hosts it, such as a webContents, rather
  // than by its views::View parent, so that the parent never deletes it. For
  // native views that a subclass adds to view() without owning them.
  static void SetOwnedByClient(views::View* view);

  // NativePeer:
  void TearDownNative() override;

  // views::ViewObserver:
  void OnViewBoundsChanged(views::View* observed_view) override;
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;

 private:
  std::unique_ptr<views::View> view_;

  base::WeakPtrFactory<Host> weak_factory_{this};

  bool interactive_ = true;
};

}  // namespace electron::api

namespace gin {

template <>
struct Converter<electron::api::View*> {
  static v8::MaybeLocal<v8::Value> ToV8(v8::Isolate* isolate,
                                        electron::api::View* val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::View** out);
};

template <>
struct Converter<const electron::api::View*> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     const electron::api::View** out);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_VIEW_H_
