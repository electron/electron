// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ash/style/rounded_rect_cutout_path_builder.h"
#include "gin/data_object_builder.h"
#include "gin/public/gin_embedders.h"
#include "gin/wrappable.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/locker.h"
#include "shell/common/gin_helper/node_entry_scope.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "ui/compositor/layer.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/background.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/layout_manager_base.h"
#include "ui/views/view.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"

#if BUILDFLAG(IS_MAC)
#include "shell/browser/animation_util.h"
#endif

namespace gin {

template <>
struct Converter<views::ChildLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ChildLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    electron::api::View* view = nullptr;
    if (!dict.Get("view", &view))
      return false;
    out->child_view = view->view();
    dict.Get("bounds", &out->bounds);
    out->visible = true;
    dict.Get("visible", &out->visible);
    return true;
  }
};

template <>
struct Converter<views::ProposedLayout> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::ProposedLayout* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("size", &out->host_size))
      return false;
    std::vector<views::ChildLayout> layouts;
    if (!dict.Get("layouts", &layouts))
      return false;
    out->child_layouts.assign(layouts.begin(), layouts.end());
    return true;
  }
};

template <>
struct Converter<views::LayoutOrientation> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutOrientation* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "horizontal") {
      *out = views::LayoutOrientation::kHorizontal;
    } else if (orientation == "vertical") {
      *out = views::LayoutOrientation::kVertical;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::LayoutAlignment> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::LayoutAlignment* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "start") {
      *out = views::LayoutAlignment::kStart;
    } else if (orientation == "center") {
      *out = views::LayoutAlignment::kCenter;
    } else if (orientation == "end") {
      *out = views::LayoutAlignment::kEnd;
    } else if (orientation == "stretch") {
      *out = views::LayoutAlignment::kStretch;
    } else if (orientation == "baseline") {
      *out = views::LayoutAlignment::kBaseline;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::FlexAllocationOrder> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::FlexAllocationOrder* out) {
    std::string orientation = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (orientation == "normal") {
      *out = views::FlexAllocationOrder::kNormal;
    } else if (orientation == "reverse") {
      *out = views::FlexAllocationOrder::kReverse;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<views::SizeBound> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBound& in) {
    if (in.is_bounded())
      return v8::Integer::New(isolate, in.value());
    return v8::Number::New(isolate, std::numeric_limits<double>::infinity());
  }
};

template <>
struct Converter<views::SizeBounds> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::SizeBounds& in) {
    return gin::DataObjectBuilder(isolate)
        .Set("width", in.width())
        .Set("height", in.height())
        .Build();
  }
};

template <>
struct Converter<gfx::Tween::Type> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     gfx::Tween::Type* out) {
    std::string easing = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (easing == "linear") {
      *out = gfx::Tween::LINEAR;
    } else if (easing == "ease-in") {
      *out = gfx::Tween::EASE_IN;
    } else if (easing == "ease-out") {
      *out = gfx::Tween::EASE_OUT;
    } else if (easing == "ease-in-out") {
      *out = gfx::Tween::EASE_IN_OUT;
    } else {
      return false;
    }
    return true;
  }
};
}  // namespace gin

namespace electron::api {

namespace {

class JSLayoutManager : public views::LayoutManagerBase {
 public:
  explicit JSLayoutManager(base::WeakPtr<View::Host> host)
      : host_(std::move(host)) {}
  ~JSLayoutManager() override = default;

  // views::LayoutManagerBase
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override {
    if (!host_)
      return {};
    return host_->CalculateProposedLayout(size_bounds);
  }

 private:
  base::WeakPtr<View::Host> host_;
};

cppgc::AllocationHandle& GetAllocationHandle() {
  return JavascriptEnvironment::GetIsolate()
      ->GetCppHeap()
      ->GetAllocationHandle();
}

}  // namespace

const gin::WrapperInfo View::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronView);

View::Host::Host(View* wrapper, std::unique_ptr<views::View> view)
    : NativePeer<View>(wrapper), view_(std::move(view)) {
  view_->set_owned_by_client(views::View::OwnedByClientPassKey{});
  view_->AddObserver(this);
  StartObservingShutdown();
}

View::Host::~Host() = default;

// static
void View::Host::SetOwnedByClient(views::View* view) {
  view->set_owned_by_client(views::View::OwnedByClientPassKey{});
}

views::ProposedLayout View::Host::CalculateProposedLayout(
    const views::SizeBounds& size_bounds) {
  auto api_view = wrapper();
  if (!api_view)
    return {};
  return api_view->CalculateProposedLayout(JavascriptEnvironment::GetIsolate(),
                                           size_bounds);
}

void View::Host::TearDownNative() {
  weak_factory_.InvalidateWeakPtrs();
  if (!view_)
    return;
  view_->RemoveObserver(this);
  if (views::View* parent = view_->parent())
    parent->RemoveChildView(view_.get());
  view_.reset();
}

void View::Host::OnViewBoundsChanged(views::View* observed_view) {
  if (auto api_view = wrapper())
    api_view->OnBoundsChanged();
}

void View::Host::OnChildViewRemoved(views::View* observed_view,
                                    views::View* child) {
  if (auto api_view = wrapper())
    api_view->OnChildViewRemoved(child);
}

View::ChildEntry::ChildEntry(View* view, ChildEntry* next)
    : view(view), next(next) {}

View::ChildEntry::~ChildEntry() = default;

void View::ChildEntry::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(view);
  visitor->Trace(next);
}

View::View() : View(std::make_unique<views::View>()) {}

View::View(std::unique_ptr<views::View> view) {
  SetHost(NativePeer<View>::Create<Host>(this, std::move(view)));
}

View::View(DeferHost) {}

View::~View() = default;

void View::SetHost(std::unique_ptr<Host, NativePeerBase::Deleter> host) {
  DCHECK(!host_);
  host_ = std::move(host);
}

views::View* View::view() const {
  return host_ ? host_->view() : nullptr;
}

bool View::IsUsable() const {
  return true;
}

views::View* View::live_view() const {
  return host_ && host_->is_active() && IsUsable() ? host_->view() : nullptr;
}

size_t View::GetChildCount() const {
  size_t count = 0;
  for (const ChildEntry* entry = first_child_; entry; entry = entry->next)
    ++count;
  return count;
}

void View::InsertChild(View* child, size_t index) {
  InsertChildEntry(cppgc::MakeGarbageCollected<ChildEntry>(
                       GetAllocationHandle(), child, nullptr),
                   index);
}

void View::InsertChildEntry(ChildEntry* entry, size_t index) {
  cppgc::Member<ChildEntry>* link = &first_child_;
  for (; index > 0 && *link; --index)
    link = &(*link)->next;
  entry->next = *link;
  *link = entry;
}

View::ChildEntry* View::TakeChildEntry(views::View* child_view) {
  if (!child_view)
    return nullptr;
  for (cppgc::Member<ChildEntry>* link = &first_child_; *link;
       link = &(*link)->next) {
    ChildEntry* entry = *link;
    if (entry->view->view() == child_view) {
      *link = entry->next;
      entry->next = nullptr;
      return entry;
    }
  }
  return nullptr;
}

void View::AddChildViewAt(gin_helper::ErrorThrower thrower,
                          View* child,
                          std::optional<size_t> maybe_index) {
  views::View* view = live_view();
  if (!view)
    return;

  views::View* child_view = child->live_view();
  if (!child_view) {
    thrower.ThrowError("Can't add a destroyed child view to a parent view");
    return;
  }

  // This will CHECK and crash in View::AddChildViewAtImpl if not handled here.
  if (view == child_view) {
    thrower.ThrowError("A view cannot be added as its own child");
    return;
  }

  const size_t count = GetChildCount();
  const size_t index = std::min(count, maybe_index.value_or(count));

  // If the child is already a child of this view, just reorder it.
  // This matches the behavior of View::AddChildViewAtImpl and
  // otherwise will CHECK if the same view is added multiple times.
  if (child_view->parent() == view) {
    // Stay among the JavaScript children, which come before any native child
    // of a subclass, such as a WebContentsView's page.
    const size_t last = count - 1;
    view->ReorderChildView(child_view, std::min(index, last));
    if (ChildEntry* entry = TakeChildEntry(child_view))
      InsertChildEntry(entry, std::min(index, last));
    return;
  }

  InsertChild(child, index);
#if BUILDFLAG(IS_MAC)
  // Disable the implicit CALayer animations that happen by default when adding
  // or removing sublayers.
  // See
  // https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/ReactingtoLayerChanges/ReactingtoLayerChanges.html
  // and https://github.com/electron/electron/pull/14911
  // TODO(nornagon): Disabling these CALayer animations (which are specific to
  // WebContentsView, I think) seems like this is something that remote_cocoa
  // or views should be taking care of, but isn't. This should be pushed
  // upstream.
  ScopedCAActionDisabler disable_animations;
#endif
  view->AddChildViewAt(child_view, index);
}

void View::RemoveChildView(View* child) {
  views::View* view = live_view();
  if (!view)
    return;

  views::View* child_view = child->view();
  if (!TakeChildEntry(child_view))
    return;
#if BUILDFLAG(IS_MAC)
  ScopedCAActionDisabler disable_animations;
#endif
  view->RemoveChildView(child_view);
}

void View::OnChildViewRemoved(views::View* child) {
  for (cppgc::Member<ChildEntry>* link = &first_child_; *link;) {
    if ((*link)->view->view() == child)
      *link = (*link)->next;
    else
      link = &(*link)->next;
  }
}

void View::OnBoundsChanged() {
  ApplyBorderRadius();
  Emit("bounds-changed");
}

ui::Layer* View::GetLayer() {
  views::View* view = live_view();
  if (!view)
    return nullptr;

  if (view->layer())
    return view->layer();

  view->SetPaintToLayer();

  ui::Layer* layer = view->layer();

  layer->SetFillsBoundsOpaquely(false);

  return layer;
}

void View::SetBounds(const gfx::Rect& bounds, gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  bool animate = false;
  int duration = 250;
  gfx::Tween::Type easing = gfx::Tween::LINEAR;

  gin_helper::Dictionary dict;
  if (args->GetNext(&dict)) {
    v8::Local<v8::Value> animate_value;

    if (dict.Get("animate", &animate_value)) {
      if (animate_value->IsBoolean()) {
        animate = animate_value->BooleanValue(isolate);
      } else {
        animate = true;

        gin_helper::Dictionary animate_dict;
        if (gin::ConvertFromV8(isolate, animate_value, &animate_dict)) {
          animate_dict.Get("duration", &duration);
          animate_dict.Get("easing", &easing);
        }
      }
    }
  }

  if (duration < 0)
    duration = 0;

  views::View* view = live_view();
  if (!view)
    return;

  if (!animate) {
    view->SetBoundsRect(bounds);
    return;
  }

  ui::Layer* layer = GetLayer();

  gfx::Rect current_bounds = view->bounds();

  if (bounds.size() == current_bounds.size()) {
    // If the size isn't changing, we can just animate the bounds directly.

    views::AnimationBuilder()
        .SetPreemptionStrategy(
            ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
        .OnEnded(base::BindOnce(
            [](views::View* view, const gfx::Rect& final_bounds) {
              view->SetBoundsRect(final_bounds);
            },
            view, bounds))
        .Once()
        .SetDuration(base::Milliseconds(duration))
        .SetBounds(view, bounds, easing);

    return;
  }

  gfx::Rect target_size = gfx::Rect(0, 0, bounds.width(), bounds.height());
  gfx::Rect max_size =
      gfx::Rect(current_bounds.x(), current_bounds.y(),
                std::max(current_bounds.width(), bounds.width()),
                std::max(current_bounds.height(), bounds.height()));

  // if the view's size is smaller than the target size, we need to set the
  // view's bounds immediatley to the new size (not position) and set the
  // layer's clip rect to animate from there.
  if (view->width() < bounds.width() || view->height() < bounds.height()) {
    view->SetBoundsRect(max_size);

    if (layer) {
      layer->SetClipRect(
          gfx::Rect(0, 0, current_bounds.width(), current_bounds.height()));
    }
  }

  views::AnimationBuilder()
      .SetPreemptionStrategy(
          ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
      .OnEnded(base::BindOnce(
          [](views::View* view, const gfx::Rect& final_bounds,
             ui::Layer* layer) {
            view->SetBoundsRect(final_bounds);
            if (layer)
              layer->SetClipRect(gfx::Rect());
          },
          view, bounds, layer))
      .Once()
      .SetDuration(base::Milliseconds(duration))
      .SetBounds(view, bounds, easing)
      .SetClipRect(
          view, target_size,
          easing);  // We have to set the clip rect independently of the
                    // bounds, because animating the bounds of the layer
                    // will not animate the underlying view's bounds.
}

gfx::Rect View::GetBounds() const {
  views::View* view = live_view();
  if (!view)
    return {};
  return view->bounds();
}

void View::SetLayout(v8::Isolate* isolate, v8::Local<v8::Object> value) {
  views::View* view = live_view();
  if (!view)
    return;
  gin_helper::Dictionary dict(isolate, value);
  v8::Local<v8::Function> calculate_proposed_layout;
  if (dict.Get("calculateProposedLayout", &calculate_proposed_layout)) {
    layout_callback_.Reset(isolate, calculate_proposed_layout);
    view->SetLayoutManager(
        std::make_unique<JSLayoutManager>(host_->GetWeakPtr()));
  } else {
    layout_callback_.Reset();
    auto* layout =
        view->SetLayoutManager(std::make_unique<views::FlexLayout>());
    views::LayoutOrientation orientation;
    if (dict.Get("orientation", &orientation))
      layout->SetOrientation(orientation);
    views::LayoutAlignment main_axis_alignment;
    if (dict.Get("mainAxisAlignment", &main_axis_alignment))
      layout->SetMainAxisAlignment(main_axis_alignment);
    views::LayoutAlignment cross_axis_alignment;
    if (dict.Get("crossAxisAlignment", &cross_axis_alignment))
      layout->SetCrossAxisAlignment(cross_axis_alignment);
    gfx::Insets interior_margin;
    if (dict.Get("interiorMargin", &interior_margin))
      layout->SetInteriorMargin(interior_margin);
    int minimum_cross_axis_size;
    if (dict.Get("minimumCrossAxisSize", &minimum_cross_axis_size))
      layout->SetMinimumCrossAxisSize(minimum_cross_axis_size);
    bool collapse_margins;
    if (dict.Get("collapseMargins", &collapse_margins))
      layout->SetCollapseMargins(collapse_margins);
    bool include_host_insets_in_layout;
    if (dict.Get("includeHostInsetsInLayout", &include_host_insets_in_layout))
      layout->SetIncludeHostInsetsInLayout(include_host_insets_in_layout);
    bool ignore_default_main_axis_margins;
    if (dict.Get("ignoreDefaultMainAxisMargins",
                 &ignore_default_main_axis_margins))
      layout->SetIgnoreDefaultMainAxisMargins(ignore_default_main_axis_margins);
    views::FlexAllocationOrder flex_allocation_order;
    if (dict.Get("flexAllocationOrder", &flex_allocation_order))
      layout->SetFlexAllocationOrder(flex_allocation_order);
  }
}

views::ProposedLayout View::CalculateProposedLayout(
    v8::Isolate* isolate,
    const views::SizeBounds& size_bounds) {
  views::ProposedLayout layout;
  if (layout_callback_.IsEmpty())
    return layout;
  gin_helper::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Function> callback = layout_callback_.Get(isolate);
  v8::Local<v8::Context> context = callback->GetCreationContextChecked(isolate);
  v8::Context::Scope context_scope(context);
  gin_helper::NodeEntryScope node_scope(context, callback);
  v8::MicrotasksScope microtasks_scope(context,
                                       v8::MicrotasksScope::kRunMicrotasks);
  v8::Local<v8::Value> arg = gin::ConvertToV8(isolate, size_bounds);
  v8::Local<v8::Value> result;
  if (callback->Call(context, callback, 1, &arg).ToLocal(&result))
    gin::ConvertFromV8(isolate, result, &layout);
  return layout;
}

v8::Local<v8::Value> View::GetChildren(v8::Isolate* isolate) {
  v8::LocalVector<v8::Value> children(isolate);
  children.reserve(GetChildCount());
  for (ChildEntry* entry = first_child_; entry; entry = entry->next) {
    v8::Local<v8::Object> wrapper;
    if (entry->view->GetWrapper(isolate).ToLocal(&wrapper))
      children.push_back(wrapper);
  }
  return v8::Array::New(isolate, children.data(), children.size());
}

void View::SetBackgroundColor(std::optional<WrappedSkColor> color) {
  views::View* view = live_view();
  if (!view)
    return;
  view->SetBackground(color ? views::CreateSolidBackground({*color}) : nullptr);
}

void View::SetBorderRadius(int radius) {
  border_radius_ = radius;
  ApplyBorderRadius();
}

void View::ApplyBorderRadius() {
  views::View* view = live_view();
  if (!border_radius_.has_value() || !view)
    return;

  auto size = view->bounds().size();

  // Restrict border radius to the constraints set in the path builder class.
  // If the constraints are exceeded, the builder will crash.
  int radius;
  {
    float r = border_radius_.value() * 1.f;
    r = std::min(r, size.width() / 2.f);
    r = std::min(r, size.height() / 2.f);
    r = std::max(r, 0.f);
    radius = std::floor(r);
  }

  // RoundedRectCutoutPathBuilder has a minimum size of 32 x 32.
  if (radius > 0 && size.width() >= 32 && size.height() >= 32) {
    auto builder = ash::RoundedRectCutoutPathBuilder(gfx::SizeF(size));
    builder.CornerRadius(radius);
    view->SetClipPath(builder.Build());
  } else {
    view->SetClipPath(SkPath());
  }
}

void View::SetBackgroundBlur(int blur_radius) {
  if (!live_view())
    return;

  if (blur_radius < 0)
    blur_radius = 0;

  ui::Layer* layer = GetLayer();

  if (!layer)
    return;

  layer->SetBackgroundBlur(blur_radius);
}

void View::SetInteractive(bool interactive) {
  interactive_ = interactive;
  if (!view_)
    return;
  view_->SetCanProcessEventsWithinSubtree(interactive);
}

void View::SetVisible(bool visible) {
  if (views::View* view = live_view())
    view->SetVisible(visible);
}

bool View::GetVisible() const {
  views::View* view = live_view();
  return view ? view->GetVisible() : false;
}

// static
View* View::New(gin::Arguments* args) {
  if (!gin_helper::ThrowIfNotConstructCall(args))
    return nullptr;
  auto* view = cppgc::MakeGarbageCollected<View>(
      args->isolate()->GetCppHeap()->GetAllocationHandle());
  gin_helper::BindToConstructCall(args, view);
  return view;
}

// static
View* View::Create(v8::Isolate* isolate) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Function> constructor = GetConstructor(isolate, context);
  v8::Local<v8::Object> obj;
  View* view = nullptr;
  if (!constructor.IsEmpty() &&
      constructor->NewInstance(context, 0, nullptr).ToLocal(&obj)) {
    gin::ConvertFromV8(isolate, obj, &view);
  }
  return view;
}

void View::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<View>::Trace(visitor);
  visitor->Trace(first_child_);
  visitor->Trace(layout_callback_);
}

const gin::WrapperInfo* View::wrapper_info() const {
  return &kWrapperInfo;
}

const char* View::GetHumanReadableName() const {
  return "Electron / View";
}

// static
void View::FillObjectTemplate(v8::Isolate* isolate,
                              v8::Local<v8::ObjectTemplate> templ) {
  gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod<&View::AddChildViewAt>("addChildView")
      .SetMethod<&View::RemoveChildView>("removeChildView")
      .SetProperty<&View::GetChildren>("children")
      .SetMethod<&View::SetBounds>("setBounds")
      .SetMethod<&View::GetBounds>("getBounds")
      .SetMethod<&View::SetBackgroundColor>("setBackgroundColor")
      .SetMethod<&View::SetBorderRadius>("setBorderRadius")
      .SetMethod<&View::SetBackgroundBlur>("setBackgroundBlur")
      .SetMethod<&View::SetInteractive>("setInteractive")
      .SetMethod<&View::GetInteractive>("getInteractive")
      .SetMethod<&View::SetLayout>("setLayout")
      .SetMethod<&View::SetVisible>("setVisible")
      .SetMethod<&View::GetVisible>("getVisible");
}

}  // namespace electron::api

namespace gin {

v8::MaybeLocal<v8::Value> Converter<electron::api::View*>::ToV8(
    v8::Isolate* isolate,
    electron::api::View* val) {
  if (!val)
    return v8::Null(isolate);
  v8::Local<v8::Object> wrapper;
  if (!val->GetWrapper(isolate).ToLocal(&wrapper))
    return {};
  return wrapper;
}

bool Converter<electron::api::View*>::FromV8(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val,
                                             electron::api::View** out) {
  *out = nullptr;
  if (!val->IsObject())
    return false;
  v8::Local<v8::Object> obj = val.As<v8::Object>();
  if (!obj->IsApiWrapper())
    return false;
  auto* wrappable = v8::Object::Unwrap<v8::Object::Wrappable>(
      isolate, obj, v8::kObjectWrappableTagRange);
  if (!wrappable)
    return false;
  const v8::Object::WrapperTypeInfo* info = wrappable->GetWrapperTypeInfo();
  if (!info || info->type_id != gin::kEmbedderNativeGin)
    return false;
  if (static_cast<const gin::WrapperInfo*>(info)->pointer_tag !=
      static_cast<gin::WrappablePointerTag>(electron::kElectronView)) {
    return false;
  }
  *out = static_cast<electron::api::View*>(
      static_cast<gin::WrappableBase*>(wrappable));
  return true;
}

bool Converter<const electron::api::View*>::FromV8(
    v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    const electron::api::View** out) {
  electron::api::View* view = nullptr;
  if (!Converter<electron::api::View*>::FromV8(isolate, val, &view))
    return false;
  *out = view;
  return true;
}

}  // namespace gin

namespace {

using electron::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("View", View::GetConstructor(isolate, context));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_view, Initialize)
