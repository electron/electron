// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents_view.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/task/sequenced_task_runner.h"
#include "base/timer/elapsed_timer.h"
#include "content/public/browser/render_frame_host.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/draggable_region_debugger.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_targeter.h"
#include "ui/views/view_targeter_delegate.h"
#include "ui/views/widget/widget.h"

namespace {

class IgnoreMouseEventsTargeterDelegate : public views::ViewTargeterDelegate {
 public:
  bool DoesIntersectRect(const views::View*,
                         const gfx::Rect&) const override {
    return false;
  }
};

}  // namespace

namespace electron::api {

WebContentsView::WebContentsView(v8::Isolate* isolate,
                                 WebContents* web_contents)
    : View(web_contents->inspectable_web_contents()->GetView()),
      api_web_contents_(web_contents) {
  set_delete_view(false);
  // See OnContentsBoundsChanging().
  web_contents->inspectable_web_contents()->GetView()->SetBoundsChangedCallback(
      base::BindRepeating(&WebContentsView::OnContentsBoundsChanging,
                          weak_factory_.GetWeakPtr()));
  view()->SetProperty(
      views::kFlexBehaviorKey,
      views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                               views::MaximumFlexSizeRule::kUnbounded));
  Observe(web_contents->web_contents());
}

WebContentsView::~WebContentsView() {
  StopObservingWindow();
  if (auto* web_contents = GetLiveWebContents())
    web_contents->Destroy();
}

WebContents* WebContentsView::GetWebContents() {
  return api_web_contents_.Get();
}

WebContents* WebContentsView::GetLiveWebContents() const {
  WebContents* web_contents = api_web_contents_.Get();
  return web_contents && !web_contents->IsDestroyed() &&
                 web_contents->web_contents()
             ? web_contents
             : nullptr;
}

void WebContentsView::SetBackgroundColor(std::optional<WrappedSkColor> color) {
  View::SetBackgroundColor(color);
  if (auto* web_contents = GetLiveWebContents()) {
    web_contents->SetBackgroundColor(color);
    // Also update the web preferences object otherwise the view will be reset
    // on the next load URL call
    auto* web_preferences =
        WebContentsPreferences::From(web_contents->web_contents());
    if (web_preferences) {
      web_preferences->SetBackgroundColor(color);
    }
  }
}

void WebContentsView::SetBorderRadius(int radius) {
  View::SetBorderRadius(radius);
  ApplyBorderRadius();
}

void WebContentsView::SetIgnoreMouseEvents(bool ignore) {
  if (!view() || ignore_mouse_events_ == ignore)
    return;

  ignore_mouse_events_ = ignore;
  if (ignore) {
    auto targeter = std::make_unique<views::ViewTargeter>(
        std::make_unique<IgnoreMouseEventsTargeterDelegate>());
    previous_event_targeter_ = view()->SetEventTargeter(std::move(targeter));
  } else {
    view()->SetEventTargeter(std::move(previous_event_targeter_));
  }
}

void WebContentsView::ApplyBorderRadius() {
  if (auto* web_contents = GetLiveWebContents();
      border_radius().has_value() && web_contents && view()->GetWidget()) {
    auto* view = web_contents->inspectable_web_contents()->GetView();
    view->SetCornerRadii(gfx::RoundedCornersF(border_radius().value()));
  }
}

int WebContentsView::NonClientHitTest(const gfx::Point& point) {
  if (!view() || !view()->GetVisible() || ignore_mouse_events_)
    return HTNOWHERE;
  if (auto* web_contents = GetLiveWebContents()) {
    auto* iwc = web_contents->inspectable_web_contents();
    if (!iwc)
      return HTNOWHERE;
    // Convert the point to the contents view's coordinate space rather than
    // the InspectableWebContentsView's coordinate space, because the draggable
    // region is relative to the web content area. When DevTools is docked
    // (e.g. to the left), the contents view is offset within the parent,
    // so we need to account for that offset.
    auto* inspectable_view = iwc->GetView();
    if (!inspectable_view)
      return HTNOWHERE;
    auto* contents_view = inspectable_view->GetContentsView();
    gfx::Point local_point(point);
    views::View::ConvertPointFromWidget(contents_view, &local_point);
    SkRegion* region = web_contents->draggable_region();
    if (region) {
      auto* debugger = web_contents->draggable_region_debugger();
      std::optional<base::ElapsedTimer> timer;
      if (debugger)
        timer.emplace();
      const bool hit = region->contains(local_point.x(), local_point.y());
      if (debugger)
        debugger->OnHitTest(timer->Elapsed(), hit);
      if (hit)
        return HTCAPTION;
    }
  }

  return HTNOWHERE;
}

void WebContentsView::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
}

void WebContentsView::OnViewAddedToWidget(views::View* observed_view) {
  DCHECK_EQ(observed_view, view());

  NativeWindow* native_window = NativeWindow::FromWidget(view()->GetWidget());
  if (!native_window)
    return;
  WebContents* web_contents = GetLiveWebContents();
  if (!web_contents)
    return;

  // We don't need to call SetOwnerWindow(nullptr) in OnViewRemovedFromWidget
  // because that's handled in the WebContents dtor called prior.
  web_contents->SetOwnerWindow(native_window);
  native_window->AddDraggableRegionProvider(this);
  StopObservingWindow();
  observed_window_ = native_window->GetWeakPtr();
  native_window->AddObserver(this);
  ApplyBorderRadius();
  if (HasLivePage())
    ScheduleWindowControlsOverlayUpdate();
}

void WebContentsView::OnViewRemovedFromWidget(views::View* observed_view) {
  DCHECK_EQ(observed_view, view());

  StopObservingWindow();

  NativeWindow* native_window = NativeWindow::FromWidget(view()->GetWidget());
  if (!native_window)
    return;

  native_window->RemoveDraggableRegionProvider(this);
}

// Our bounds changed and the RenderWidgetHostView is about to be resized to
// match. Push the re-clipped overlay rect now so that it rides along with the
// resize in a single VisualProperties update, rather than trailing it (where it
// could sit behind the resize's pending ack).
void WebContentsView::OnContentsBoundsChanging() {
  if (HasLivePage())
    SendWindowControlsOverlay();
}

bool WebContentsView::HasLivePage() {
  // Before the first navigation there is nothing to update; the window
  // notifies us again from WebContents::DidFinishNavigation.
  return observed_window_ && web_contents() &&
         web_contents()->GetPrimaryMainFrame()->IsRenderFrameLive();
}

// NativeWindowObserver. This fires from inside the frame view's layout, before
// the client area (and so this view) has been laid out, so defer until the
// current layout pass has finished to avoid clipping against stale bounds.
void WebContentsView::UpdateWindowControlsOverlay(
    const gfx::Rect& bounding_rect) {
  ScheduleWindowControlsOverlayUpdate();
}

void WebContentsView::ScheduleWindowControlsOverlayUpdate() {
  if (window_controls_overlay_update_pending_)
    return;
  window_controls_overlay_update_pending_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&WebContentsView::SendWindowControlsOverlay,
                                weak_factory_.GetWeakPtr()));
}

// The overlay rect is relative to the window's content area. Translate it
// into this view's coordinates so that views which only partially cover (or
// don't cover) the titlebar report the right env(titlebar-area-*) values.
void WebContentsView::SendWindowControlsOverlay() {
  window_controls_overlay_update_pending_ = false;
  WebContents* api_web_contents = GetLiveWebContents();
  if (!api_web_contents || !observed_window_)
    return;
  const auto bounding_rect = observed_window_->GetWindowControlsOverlayRect();
  if (!bounding_rect)
    return;
  views::View* window_view = observed_window_->GetContentsView();
  if (!window_view || !window_view->Contains(view()))
    return;

  gfx::Rect local_rect =
      views::View::ConvertRectToTarget(window_view, view(), *bounding_rect);
  local_rect.Intersect(view()->GetLocalBounds());
  api_web_contents->web_contents()->UpdateWindowControlsOverlay(local_rect);
}

void WebContentsView::StopObservingWindow() {
  if (observed_window_)
    observed_window_->RemoveObserver(this);
  observed_window_ = nullptr;
}

// static
gin_helper::Handle<WebContentsView> WebContentsView::Create(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& web_preferences) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> arg = gin::DataObjectBuilder(isolate)
                                 .Set("webPreferences", web_preferences)
                                 .Build();
  v8::Local<v8::Object> web_contents_view_obj;
  if (GetConstructor(isolate)
          ->NewInstance(context, 1, &arg)
          .ToLocal(&web_contents_view_obj)) {
    gin_helper::Handle<WebContentsView> web_contents_view;
    if (gin::ConvertFromV8(isolate, web_contents_view_obj, &web_contents_view))
      return web_contents_view;
  }
  return {};
}

// static
v8::Local<v8::Function> WebContentsView::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(isolate,
                       gin_helper::CreateConstructor<WebContentsView>(
                           isolate, base::BindRepeating(&WebContentsView::New),
                           View::GetConstructorTemplate(isolate)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// static
gin_helper::WrappableBase* WebContentsView::New(gin::Arguments* const args) {
  v8::Isolate* const isolate = args->isolate();
  gin_helper::Dictionary web_preferences;
  v8::Local<v8::Value> existing_web_contents_value;
  {
    v8::Local<v8::Value> options_value;
    if (args->GetNext(&options_value)) {
      gin_helper::Dictionary options;
      if (!gin::ConvertFromV8(isolate, options_value, &options)) {
        args->ThrowTypeError("options must be an object");
        return nullptr;
      }
      v8::Local<v8::Value> web_preferences_value;
      if (options.Get("webPreferences", &web_preferences_value)) {
        if (!gin::ConvertFromV8(isolate, web_preferences_value,
                                &web_preferences)) {
          args->ThrowTypeError("options.webPreferences must be an object");
          return nullptr;
        }
      }

      if (options.Get("webContents", &existing_web_contents_value)) {
        WebContents* existing_web_contents = nullptr;
        if (!gin::ConvertFromV8(isolate, existing_web_contents_value,
                                &existing_web_contents)) {
          args->ThrowTypeError("options.webContents must be a WebContents");
          return nullptr;
        }

        if (existing_web_contents->owner_window() != nullptr) {
          args->ThrowTypeError(
              "options.webContents is already attached to a window");
          return nullptr;
        }
      }
    }
  }

  if (web_preferences.IsEmpty())
    web_preferences = gin_helper::Dictionary::CreateEmpty(isolate);
  if (!web_preferences.Has(options::kShow))
    web_preferences.Set(options::kShow, false);

  if (!existing_web_contents_value.IsEmpty()) {
    web_preferences.SetHidden("webContents", existing_web_contents_value);
  }

  auto* web_contents =
      WebContents::CreateFromWebPreferences(isolate, web_preferences);

  // Constructor call.
  auto* view = new WebContentsView{isolate, web_contents};
  view->InitWithArgs(args);
  return view;
}

// static
void WebContentsView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "WebContentsView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod<&WebContentsView::SetBackgroundColor>("setBackgroundColor")
      .SetMethod<&WebContentsView::SetBorderRadius>("setBorderRadius")
      .SetMethod<&WebContentsView::SetIgnoreMouseEvents>("setIgnoreMouseEvents")
      .SetProperty<&WebContentsView::GetWebContents>("webContents");
}

}  // namespace electron::api

namespace {

using electron::api::WebContentsView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("WebContentsView", WebContentsView::GetConstructor(isolate));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_web_contents_view,
                                  Initialize)
