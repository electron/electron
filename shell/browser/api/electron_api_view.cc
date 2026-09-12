// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_view.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "ash/style/rounded_rect_cutout_path_builder.h"
#include "base/auto_reset.h"
#include "gin/data_object_builder.h"
#include "gin/wrappable.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "ui/compositor/layer.h"
#include "ui/views/animation/animation_builder.h"
#include "ui/views/background.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/flex_layout.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/layout/layout_manager_base.h"
#include "ui/views/view_class_properties.h"

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
    gin_helper::Handle<electron::api::View> view;
    if (!dict.Get("view", &view))
      return false;
    out->child_view = view->view();
    if (dict.Has("bounds"))
      dict.Get("bounds", &out->bounds);
    out->visible = true;
    if (dict.Has("visible"))
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
    if (!dict.Get("layouts", &out->child_layouts))
      return false;
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
struct Converter<views::MinimumFlexSizeRule> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::MinimumFlexSizeRule* out) {
    std::string s = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (s == "scale-to-zero") {
      *out = views::MinimumFlexSizeRule::kScaleToZero;
    } else if (s == "scale-to-minimum-snap-to-zero") {
      *out = views::MinimumFlexSizeRule::kScaleToMinimumSnapToZero;
    } else if (s == "preferred-snap-to-zero") {
      *out = views::MinimumFlexSizeRule::kPreferredSnapToZero;
    } else if (s == "scale-to-minimum") {
      *out = views::MinimumFlexSizeRule::kScaleToMinimum;
    } else if (s == "preferred-snap-to-minimum") {
      *out = views::MinimumFlexSizeRule::kPreferredSnapToMinimum;
    } else if (s == "preferred") {
      *out = views::MinimumFlexSizeRule::kPreferred;
    } else {
      return false;
    }
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   views::MinimumFlexSizeRule in) {
    switch (in) {
      case views::MinimumFlexSizeRule::kScaleToZero:
        return gin::StringToV8(isolate, "scale-to-zero");
      case views::MinimumFlexSizeRule::kScaleToMinimumSnapToZero:
        return gin::StringToV8(isolate, "scale-to-minimum-snap-to-zero");
      case views::MinimumFlexSizeRule::kPreferredSnapToZero:
        return gin::StringToV8(isolate, "preferred-snap-to-zero");
      case views::MinimumFlexSizeRule::kScaleToMinimum:
        return gin::StringToV8(isolate, "scale-to-minimum");
      case views::MinimumFlexSizeRule::kPreferredSnapToMinimum:
        return gin::StringToV8(isolate, "preferred-snap-to-minimum");
      case views::MinimumFlexSizeRule::kPreferred:
        return gin::StringToV8(isolate, "preferred");
    }
    return gin::StringToV8(isolate, "preferred");
  }
};

template <>
struct Converter<views::MaximumFlexSizeRule> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::MaximumFlexSizeRule* out) {
    std::string s = base::ToLowerASCII(gin::V8ToString(isolate, val));
    if (s == "preferred") {
      *out = views::MaximumFlexSizeRule::kPreferred;
    } else if (s == "scale-to-maximum") {
      *out = views::MaximumFlexSizeRule::kScaleToMaximum;
    } else if (s == "unbounded") {
      *out = views::MaximumFlexSizeRule::kUnbounded;
    } else {
      return false;
    }
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   views::MaximumFlexSizeRule in) {
    switch (in) {
      case views::MaximumFlexSizeRule::kPreferred:
        return gin::StringToV8(isolate, "preferred");
      case views::MaximumFlexSizeRule::kScaleToMaximum:
        return gin::StringToV8(isolate, "scale-to-maximum");
      case views::MaximumFlexSizeRule::kUnbounded:
        return gin::StringToV8(isolate, "unbounded");
    }
    return gin::StringToV8(isolate, "preferred");
  }
};

template <>
struct Converter<views::FlexSpecification> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     views::FlexSpecification* out) {
    gin_helper::Dictionary dict;
    if (!gin::ConvertFromV8(isolate, val, &dict))
      return false;
    views::MinimumFlexSizeRule min_rule =
        views::MinimumFlexSizeRule::kPreferred;
    views::MaximumFlexSizeRule max_rule =
        views::MaximumFlexSizeRule::kPreferred;
    dict.Get("minimum", &min_rule);
    dict.Get("maximum", &max_rule);
    int weight = 0;
    int order = 1;
    dict.Get("weight", &weight);
    dict.Get("order", &order);
    *out = views::FlexSpecification(min_rule, max_rule)
               .WithWeight(weight)
               .WithOrder(order);
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const views::FlexSpecification& in) {
    return gin::DataObjectBuilder(isolate)
        .Set("weight", in.weight())
        .Set("order", in.order())
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

using LayoutCallback = base::RepeatingCallback<views::ProposedLayout(
    const views::SizeBounds& size_bounds)>;

class JSLayoutManager : public views::LayoutManagerBase {
 public:
  explicit JSLayoutManager(LayoutCallback layout_callback)
      : layout_callback_(std::move(layout_callback)) {}
  ~JSLayoutManager() override = default;

  bool is_dispatching() const { return is_dispatching_; }

  // views::LayoutManagerBase
  views::ProposedLayout CalculateProposedLayout(
      const views::SizeBounds& size_bounds) const override {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    // is_dispatching_ blocks re-entrant setLayout(): swapping the layout
    // manager mid-callback would destroy `this` while it is still on the
    // stack.
    base::AutoReset<bool> reset(&is_dispatching_, true);
    // Exceptions thrown from the JS callback are absorbed by gin's
    // V8FunctionInvoker — it returns a default-constructed ProposedLayout
    // and leaves the exception pending on the isolate. V8 observes it when
    // control next returns to JS; Node then decides whether to surface it
    // via uncaughtException or to swallow at the binding boundary.
    return layout_callback_.Run(size_bounds);
  }

 private:
  LayoutCallback layout_callback_;
  mutable bool is_dispatching_ = false;
};

namespace {
std::string AlignmentToString(views::LayoutAlignment a) {
  switch (a) {
    case views::LayoutAlignment::kStart:
      return "start";
    case views::LayoutAlignment::kCenter:
      return "center";
    case views::LayoutAlignment::kEnd:
      return "end";
    case views::LayoutAlignment::kStretch:
      return "stretch";
    case views::LayoutAlignment::kBaseline:
      return "baseline";
  }
  return "";
}
}  // namespace

View::View(views::View* view) : view_(view) {
  view_->set_owned_by_client(views::View::OwnedByClientPassKey{});
  view_->AddObserver(this);
}

View::View() : View(new views::View()) {}

View::~View() {
  if (!view_)
    return;
  view_->RemoveObserver(this);
  if (delete_view_)
    view_.ClearAndDelete();
}

void View::ReorderChildView(gin_helper::Handle<View> child, size_t index) {
  view_->ReorderChildView(child->view(), index);

  const auto i =
      std::ranges::find_if(child_views_, [&](const ChildPair& child_view) {
        return child_view.first == child->view();
      });
  DCHECK(i != child_views_.end());

  // If |view| is already at the desired position, there's nothing to do.
  const auto pos = std::next(
      child_views_.begin(),
      static_cast<ptrdiff_t>(std::min(index, child_views_.size() - 1)));
  if (i == pos)
    return;

  if (pos < i) {
    std::rotate(pos, i, std::next(i));
  } else {
    std::rotate(i, std::next(i), std::next(pos));
  }
}

void View::AddChildViewAt(gin_helper::Handle<View> child,
                          std::optional<size_t> maybe_index) {
  // TODO(nornagon): !view_ is only for supporting the weird case of
  // WebContentsView's view being deleted when the underlying WebContents is
  // destroyed (on non-Mac). We should fix that so that WebContentsView always
  // has a View, possibly a wrapper view around the underlying platform View.
  if (!view_)
    return;

  if (!child->view()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "Can't add a destroyed child view to a parent view");
    return;
  }

  // This will CHECK and crash in View::AddChildViewAtImpl if not handled here.
  if (view_ == child->view()) {
    gin_helper::ErrorThrower(isolate()).ThrowError(
        "A view cannot be added as its own child");
    return;
  }

  size_t index =
      std::min(child_views_.size(), maybe_index.value_or(child_views_.size()));

  // If the child is already a child of this view, just reorder it.
  // This matches the behavior of View::AddChildViewAtImpl and
  // otherwise will CHECK if the same view is added multiple times.
  if (child->view()->parent() == view_) {
    ReorderChildView(child, index);
    return;
  }

  child_views_.emplace(child_views_.begin() + index,  // index
                       child->view(),
                       v8::Global<v8::Object>(isolate(), child->GetWrapper()));
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
  view_->AddChildViewAt(child->view(), index);
}

void View::RemoveChildView(gin_helper::Handle<View> child) {
  if (!view_)
    return;

  const auto it =
      std::ranges::find_if(child_views_, [&](const ChildPair& child_view) {
        return child_view.first == child->view();
      });
  if (it != child_views_.end()) {
#if BUILDFLAG(IS_MAC)
    ScopedCAActionDisabler disable_animations;
#endif
    // Remove from child_views first so that OnChildViewRemoved doesn't try to
    // remove it again
    child_views_.erase(it);
    // It's possible for the child's view to be invalid here
    // if the child's webContents was closed or destroyed.
    if (child->view())
      view_->RemoveChildView(child->view());
  }
}

ui::Layer* View::GetLayer() {
  if (!view_)
    return nullptr;

  if (view_->layer())
    return view_->layer();

  view_->SetPaintToLayer();

  ui::Layer* layer = view_->layer();

  layer->SetFillsBoundsOpaquely(false);

  return layer;
}

void View::SetBounds(const gfx::Rect& bounds, gin::Arguments* const args) {
  bool animate = false;
  int duration = 250;
  gfx::Tween::Type easing = gfx::Tween::LINEAR;

  gin_helper::Dictionary dict;
  if (args->GetNext(&dict)) {
    v8::Local<v8::Value> animate_value;

    if (dict.Get("animate", &animate_value)) {
      if (animate_value->IsBoolean()) {
        animate = animate_value->BooleanValue(isolate());
      } else {
        animate = true;

        gin_helper::Dictionary animate_dict;
        if (gin::ConvertFromV8(isolate(), animate_value, &animate_dict)) {
          animate_dict.Get("duration", &duration);
          animate_dict.Get("easing", &easing);
        }
      }
    }
  }

  if (duration < 0)
    duration = 0;

  if (!view_)
    return;

  if (!animate) {
    view_->SetBoundsRect(bounds);
    return;
  }

  ui::Layer* layer = GetLayer();

  gfx::Rect current_bounds = view_->bounds();

  if (bounds.size() == current_bounds.size()) {
    // If the size isn't changing, we can just animate the bounds directly.

    views::AnimationBuilder()
        .SetPreemptionStrategy(
            ui::LayerAnimator::IMMEDIATELY_ANIMATE_TO_NEW_TARGET)
        .OnEnded(base::BindOnce(
            [](views::View* view, const gfx::Rect& final_bounds) {
              view->SetBoundsRect(final_bounds);
            },
            view_, bounds))
        .Once()
        .SetDuration(base::Milliseconds(duration))
        .SetBounds(view_, bounds, easing);

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
  if (view_->width() < bounds.width() || view_->height() < bounds.height()) {
    view_->SetBoundsRect(max_size);

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
          view_, bounds, layer))
      .Once()
      .SetDuration(base::Milliseconds(duration))
      .SetBounds(view_, bounds, easing)
      .SetClipRect(
          view_, target_size,
          easing);  // We have to set the clip rect independently of the
                    // bounds, because animating the bounds of the layer
                    // will not animate the underlying view's bounds.
}

gfx::Rect View::GetBounds() const {
  if (!view_)
    return {};
  return view_->bounds();
}

void View::SetLayout(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  if (!view_)
    return;

  // Re-entrancy guard: setLayout cannot be called from inside a JS layout
  // callback because it would replace the layout manager mid-dispatch.
  if (js_layout_manager_ && js_layout_manager_->is_dispatching()) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Cannot call setLayout from inside calculateProposedLayout");
    return;
  }

  // null/undefined => clear the layout manager.
  if (value.IsEmpty() || value->IsNullOrUndefined()) {
    js_layout_manager_ = nullptr;
    layout_type_ = LayoutType::kNone;
    view_->SetLayoutManager(nullptr);
    return;
  }

  // Reject primitives explicitly (V8's ToObject() auto-wraps Number/String
  // into objects, which would silently fall through to a default FlexLayout).
  if (!value->IsObject() || value->IsArray() || value->IsFunction()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Layout options must be a plain object or null");
    return;
  }
  v8::Local<v8::Object> obj = value.As<v8::Object>();
  gin_helper::Dictionary dict(isolate, obj);

  // Custom JS layout callback path.
  LayoutCallback calculate_proposed_layout;
  if (dict.Get("calculateProposedLayout", &calculate_proposed_layout)) {
    auto lm =
        std::make_unique<JSLayoutManager>(std::move(calculate_proposed_layout));
    js_layout_manager_ = lm.get();
    layout_type_ = LayoutType::kJs;
    view_->SetLayoutManager(std::move(lm));
    return;
  }

  // Replacing any prior JSLayoutManager with a built-in.
  js_layout_manager_ = nullptr;

  std::string type;
  dict.Get("type", &type);

  if (type == "fill") {
    layout_type_ = LayoutType::kFill;
    auto* fill_layout =
        view_->SetLayoutManager(std::make_unique<views::FillLayout>());
    bool minimum_size_enabled;
    if (dict.Has("minimumSizeEnabled") &&
        dict.Get("minimumSizeEnabled", &minimum_size_enabled))
      fill_layout->SetMinimumSizeEnabled(minimum_size_enabled);
    bool include_insets;
    if (dict.Has("includeInsets") && dict.Get("includeInsets", &include_insets))
      fill_layout->SetIncludeInsets(include_insets);
    return;
  }

  if (type == "box") {
    views::LayoutOrientation orientation =
        views::LayoutOrientation::kHorizontal;
    dict.Get("orientation", &orientation);
    layout_type_ = LayoutType::kBox;
    auto* layout = view_->SetLayoutManager(
        std::make_unique<views::BoxLayout>(orientation));
    int between_child_spacing;
    if (dict.Get("betweenChildSpacing", &between_child_spacing))
      layout->set_between_child_spacing(between_child_spacing);
    gfx::Insets inside_border_insets;
    if (dict.Get("insideBorderInsets", &inside_border_insets))
      layout->set_inside_border_insets(inside_border_insets);
    views::LayoutAlignment box_main_axis;
    if (dict.Get("mainAxisAlignment", &box_main_axis))
      layout->set_main_axis_alignment(box_main_axis);
    views::LayoutAlignment box_cross_axis;
    if (dict.Get("crossAxisAlignment", &box_cross_axis))
      layout->set_cross_axis_alignment(box_cross_axis);
    int minimum_cross_axis_size;
    if (dict.Get("minimumCrossAxisSize", &minimum_cross_axis_size))
      layout->set_minimum_cross_axis_size(minimum_cross_axis_size);
    int default_flex;
    if (dict.Get("defaultFlex", &default_flex))
      layout->SetDefaultFlex(default_flex);
    return;
  }

  // An explicit `type` we don't recognise is an error. An *absent* `type`
  // falls through to FlexLayout, preserving backward compatibility with
  // the legacy setLayout({orientation: ...}) shape from PR #35658.
  if (!type.empty() && type != "flex") {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Unknown layout type: '" + type +
        "'. Expected one of 'fill', 'box', 'flex', or omit the field.");
    return;
  }

  // type == "flex" or omitted: configure a FlexLayout.
  layout_type_ = LayoutType::kFlex;
  auto* layout = view_->SetLayoutManager(std::make_unique<views::FlexLayout>());
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
  if (dict.Has("collapseMargins") &&
      dict.Get("collapseMargins", &collapse_margins))
    layout->SetCollapseMargins(collapse_margins);
  bool include_host_insets_in_layout;
  if (dict.Has("includeHostInsetsInLayout") &&
      dict.Get("includeHostInsetsInLayout", &include_host_insets_in_layout))
    layout->SetIncludeHostInsetsInLayout(include_host_insets_in_layout);
  bool ignore_default_main_axis_margins;
  if (dict.Has("ignoreDefaultMainAxisMargins") &&
      dict.Get("ignoreDefaultMainAxisMargins",
               &ignore_default_main_axis_margins))
    layout->SetIgnoreDefaultMainAxisMargins(ignore_default_main_axis_margins);
  views::FlexAllocationOrder flex_allocation_order;
  if (dict.Get("flexAllocationOrder", &flex_allocation_order))
    layout->SetFlexAllocationOrder(flex_allocation_order);
}

void View::SetLayoutFlex(v8::Isolate* isolate, v8::Local<v8::Value> spec) {
  if (!view_)
    return;
  if (spec.IsEmpty() || spec->IsNullOrUndefined()) {
    view_->ClearProperty(views::kFlexBehaviorKey);
    last_min_flex_rule_.reset();
    last_max_flex_rule_.reset();
    view_->InvalidateLayout();
    return;
  }
  views::FlexSpecification fs;
  if (!gin::ConvertFromV8(isolate, spec, &fs)) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "Invalid flex specification");
    return;
  }
  // Cache min/max so getLayoutFlex can return them — FlexSpecification has no
  // accessor for these once they're baked into its FlexRule closure.
  gin_helper::Dictionary dict;
  if (gin::ConvertFromV8(isolate, spec, &dict)) {
    views::MinimumFlexSizeRule cached_min;
    if (dict.Get("minimum", &cached_min))
      last_min_flex_rule_ = cached_min;
    else
      last_min_flex_rule_.reset();
    views::MaximumFlexSizeRule cached_max;
    if (dict.Get("maximum", &cached_max))
      last_max_flex_rule_ = cached_max;
    else
      last_max_flex_rule_.reset();
  }
  view_->SetProperty(views::kFlexBehaviorKey, fs);
  view_->InvalidateLayout();
}

void View::SetLayoutMargins(const gfx::Insets& margins) {
  if (!view_)
    return;
  view_->SetProperty(views::kMarginsKey, margins);
  view_->InvalidateLayout();
}

void View::Layout() {
  if (!view_)
    return;
  // Force a synchronous layout pass. Useful in tests and for advanced
  // consumers that have just mutated layout-affecting properties and
  // want the new bounds reflected immediately.
  view_->DeprecatedLayoutImmediately();
}

void View::InvalidateLayout() {
  if (!view_)
    return;
  view_->InvalidateLayout();
}

void View::SetPreferredSize(const gfx::Size& size) {
  if (!view_)
    return;
  view_->SetPreferredSize(size);
  view_->InvalidateLayout();
}

gfx::Size View::GetPreferredSize() const {
  if (!view_)
    return {};
  return view_->GetPreferredSize();
}

void View::SetUseDefaultFillLayout(bool use_default) {
  if (!view_)
    return;
  view_->SetUseDefaultFillLayout(use_default);
}

std::string View::GetLayoutManagerType() const {
  switch (layout_type_) {
    case LayoutType::kFill:
      return "fill";
    case LayoutType::kBox:
      return "box";
    case LayoutType::kFlex:
      return "flex";
    case LayoutType::kJs:
      return "js";
    case LayoutType::kNone:
    default:
      return "";
  }
}

void View::SetBoxFlex(v8::Isolate* isolate,
                      gin_helper::Handle<View> child,
                      int weight,
                      gin::Arguments* args) {
  if (!view_ || !child->view())
    return;
  if (layout_type_ != LayoutType::kBox) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "setBoxFlex requires a box layout. Call setLayout({type: 'box'}) "
        "first.");
    return;
  }
  if (child->view()->parent() != view_) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "setBoxFlex target must be a direct child of this view");
    return;
  }
  bool use_min_size = false;
  args->GetNext(&use_min_size);
  auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
  if (!layout)
    return;
  layout->SetFlexForView(child->view(), weight, use_min_size);
  view_->InvalidateLayout();
}

v8::Local<v8::Value> View::GetLayoutFlex(v8::Isolate* isolate) const {
  if (!view_)
    return v8::Null(isolate);
  const views::FlexSpecification* spec =
      view_->GetProperty(views::kFlexBehaviorKey);
  if (!spec)
    return v8::Null(isolate);
  gin::DataObjectBuilder builder(isolate);
  builder.Set("weight", spec->weight()).Set("order", spec->order());
  if (last_min_flex_rule_)
    builder.Set("minimum", *last_min_flex_rule_);
  if (last_max_flex_rule_)
    builder.Set("maximum", *last_max_flex_rule_);
  return builder.Build();
}

std::string View::GetOrientation() const {
  if (!view_)
    return "";
  if (layout_type_ == LayoutType::kBox) {
    auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
    if (!layout)
      return "";
    return layout->GetOrientation() ==
                   views::BoxLayout::Orientation::kHorizontal
               ? "horizontal"
               : "vertical";
  }
  if (layout_type_ == LayoutType::kFlex) {
    auto* layout = static_cast<views::FlexLayout*>(view_->GetLayoutManager());
    if (!layout)
      return "";
    return layout->orientation() == views::LayoutOrientation::kHorizontal
               ? "horizontal"
               : "vertical";
  }
  return "";
}

std::string View::GetMainAxisAlignment() const {
  if (!view_)
    return "";
  if (layout_type_ == LayoutType::kBox) {
    auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
    return layout ? AlignmentToString(layout->main_axis_alignment()) : "";
  }
  if (layout_type_ == LayoutType::kFlex) {
    auto* layout = static_cast<views::FlexLayout*>(view_->GetLayoutManager());
    return layout ? AlignmentToString(layout->main_axis_alignment()) : "";
  }
  return "";
}

std::string View::GetCrossAxisAlignment() const {
  if (!view_)
    return "";
  if (layout_type_ == LayoutType::kBox) {
    auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
    return layout ? AlignmentToString(layout->cross_axis_alignment()) : "";
  }
  if (layout_type_ == LayoutType::kFlex) {
    auto* layout = static_cast<views::FlexLayout*>(view_->GetLayoutManager());
    return layout ? AlignmentToString(layout->cross_axis_alignment()) : "";
  }
  return "";
}

int View::GetBetweenChildSpacing() const {
  if (!view_ || layout_type_ != LayoutType::kBox)
    return 0;
  auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
  if (!layout)
    return 0;
  return layout->between_child_spacing();
}

gfx::Insets View::GetInsideBorderInsets() const {
  if (!view_ || layout_type_ != LayoutType::kBox)
    return gfx::Insets();
  auto* layout = static_cast<views::BoxLayout*>(view_->GetLayoutManager());
  if (!layout)
    return gfx::Insets();
  return layout->inside_border_insets();
}

std::vector<v8::Local<v8::Value>> View::GetChildren() {
  std::vector<v8::Local<v8::Value>> ret;
  ret.reserve(child_views_.size());

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  for (auto& [view, global] : child_views_)
    ret.push_back(global.Get(isolate));

  return ret;
}

void View::SetBackgroundColor(std::optional<WrappedSkColor> color) {
  if (!view_)
    return;
  view_->SetBackground(color ? views::CreateSolidBackground({*color})
                             : nullptr);
}

void View::SetBorderRadius(int radius) {
  border_radius_ = radius;
  ApplyBorderRadius();
}

void View::ApplyBorderRadius() {
  if (!border_radius_.has_value() || !view_)
    return;

  auto size = view_->bounds().size();

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
    view_->SetClipPath(builder.Build());
  } else {
    view_->SetClipPath(SkPath());
  }
}

void View::SetBackgroundBlur(int blur_radius) {
  if (!view_)
    return;

  if (blur_radius < 0)
    blur_radius = 0;

  ui::Layer* layer = GetLayer();

  if (!layer)
    return;

  layer->SetBackgroundBlur(blur_radius);
}

void View::SetVisible(bool visible) {
  if (!view_)
    return;
  view_->SetVisible(visible);
}

bool View::GetVisible() const {
  return view_ ? view_->GetVisible() : false;
}

void View::OnViewBoundsChanged(views::View* observed_view) {
  ApplyBorderRadius();
  Emit("bounds-changed");
}

void View::OnViewIsDeleting(views::View* observed_view) {
  DCHECK_EQ(observed_view, view_);
  view_ = nullptr;
}

void View::OnChildViewRemoved(views::View* observed_view, views::View* child) {
  std::erase_if(child_views_, [child](const ChildPair& child_view) {
    return child_view.first == child;
  });
}

// static
gin_helper::WrappableBase* View::New(gin::Arguments* args) {
  View* view = new View();
  view->InitWithArgs(args);
  return view;
}

// static
v8::Local<v8::Function> View::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(isolate, gin_helper::CreateConstructor<View>(
                                    isolate, base::BindRepeating(&View::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// static
gin_helper::Handle<View> View::Create(v8::Isolate* isolate) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Object> obj;
  if (GetConstructor(isolate)->NewInstance(context, 0, nullptr).ToLocal(&obj)) {
    gin_helper::Handle<View> view;
    if (gin::ConvertFromV8(isolate, obj, &view))
      return view;
  }
  return {};
}

// static
void View::BuildPrototype(v8::Isolate* isolate,
                          v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "View"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addChildView", &View::AddChildViewAt)
      .SetMethod("removeChildView", &View::RemoveChildView)
      .SetProperty("children", &View::GetChildren)
      .SetMethod("setBounds", &View::SetBounds)
      .SetMethod("getBounds", &View::GetBounds)
      .SetMethod("setBackgroundColor", &View::SetBackgroundColor)
      .SetMethod("setBorderRadius", &View::SetBorderRadius)
      .SetMethod("setBackgroundBlur", &View::SetBackgroundBlur)
      .SetMethod("setLayout", &View::SetLayout)
      .SetMethod("setLayoutFlex", &View::SetLayoutFlex)
      .SetMethod("setLayoutMargins", &View::SetLayoutMargins)
      .SetMethod("getLayoutFlex", &View::GetLayoutFlex)
      .SetMethod("layout", &View::Layout)
      .SetMethod("invalidateLayout", &View::InvalidateLayout)
      .SetMethod("setPreferredSize", &View::SetPreferredSize)
      .SetMethod("getPreferredSize", &View::GetPreferredSize)
      .SetMethod("setUseDefaultFillLayout", &View::SetUseDefaultFillLayout)
      .SetMethod("getLayoutManagerType", &View::GetLayoutManagerType)
      .SetMethod("getOrientation", &View::GetOrientation)
      .SetMethod("getMainAxisAlignment", &View::GetMainAxisAlignment)
      .SetMethod("getCrossAxisAlignment", &View::GetCrossAxisAlignment)
      .SetMethod("getBetweenChildSpacing", &View::GetBetweenChildSpacing)
      .SetMethod("getInsideBorderInsets", &View::GetInsideBorderInsets)
      .SetMethod("setBoxFlex", &View::SetBoxFlex)
      .SetMethod("setVisible", &View::SetVisible)
      .SetMethod("getVisible", &View::GetVisible);
}

}  // namespace electron::api

namespace {

using electron::api::View;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("View", View::GetConstructor(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_view, Initialize)
