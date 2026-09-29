// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/timer/elapsed_timer.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "gin/data_object_builder.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/draggable_region_provider.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/native_window.h"
#include "shell/browser/native_window_observer.h"
#include "shell/browser/ui/draggable_region_debugger.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/base/hit_test.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_observer.h"
#include "ui/views/view_tracker.h"
#include "ui/views/widget/widget.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-cppgc.h"

namespace electron::api {

namespace {

// The native view of a WebContentsView. It holds the webContents'
// InspectableWebContentsView, which the webContents owns and deletes with
// itself, so that the WebContentsView keeps one native view, and its place in
// the view tree, for its whole life. The inspectable view fills it and stays
// its last child. Child views added from JavaScript go before it, where they
// went when they were added to the inspectable view itself.
class WebContentsContainerView : public views::View,
                                 public views::ViewObserver {
  METADATA_HEADER(WebContentsContainerView, views::View)

 public:
  WebContentsContainerView() {
    // Lets a flex layout in the parent size the view, as it did the
    // inspectable view.
    SetProperty(
        views::kFlexBehaviorKey,
        views::FlexSpecification(views::MinimumFlexSizeRule::kScaleToMinimum,
                                 views::MaximumFlexSizeRule::kUnbounded));
    GetViewAccessibility().SetIsIgnored(true);
  }

  void TakeInspectableView(InspectableWebContentsView* inspectable_view) {
    // Layouts set from JavaScript arrange only the JavaScript children.
    inspectable_view->SetProperty(views::kViewIgnoredByLayoutKey, true);
    AddChildViewRaw(static_cast<views::View*>(inspectable_view));
    inspectable_view->SetBoundsRect(GetLocalBounds());
    inspectable_view_.SetView(inspectable_view);
    inspectable_view_deleting_ = false;
    inspectable_view_observation_.Reset();
    inspectable_view_observation_.Observe(inspectable_view);
  }

  // The inspectable view, while it is still here. Null once another
  // WebContentsView has adopted the webContents, or the webContents has
  // deleted it.
  InspectableWebContentsView* GetOwnedInspectableView() {
    views::View* view = inspectable_view_.view();
    return view && view->parent() == this
               ? static_cast<InspectableWebContentsView*>(view)
               : nullptr;
  }

  // Whether |child| is the inspectable view and was removed because the
  // webContents is deleting it, rather than because another WebContentsView
  // adopted the webContents.
  bool IsInspectableViewBeingDeleted(const views::View* child) const {
    return inspectable_view_deleting_ && child == inspectable_view_.view();
  }

  // views::View:
  void Layout(PassKey) override {
    LayoutSuperclass<views::View>(this);
    if (InspectableWebContentsView* view = GetOwnedInspectableView())
      view->SetBoundsRect(GetLocalBounds());
  }

 private:
  // views::ViewObserver:
  void OnViewHierarchyWillBeDeleted(views::View* observed_view) override {
    inspectable_view_deleting_ = true;
  }
  void OnViewIsDeleting(views::View* observed_view) override {
    inspectable_view_observation_.Reset();
  }

  views::ViewTracker inspectable_view_;
  bool inspectable_view_deleting_ = false;
  base::ScopedObservation<views::View, views::ViewObserver>
      inspectable_view_observation_{this};
};

BEGIN_METADATA(WebContentsContainerView)
END_METADATA

}  // namespace

class WebContentsViewHost final : public View::Host,
                                  public content::WebContentsObserver,
                                  public NativeWindowObserver,
                                  public DraggableRegionProvider {
 public:
  WebContentsViewHost(WebContentsView* wrapper, WebContents* web_contents);

  void RegisterDraggableRegionProvider(NativeWindow* window);
  void ApplyBorderRadius(std::optional<int> radius);

  // DraggableRegionProvider:
  int NonClientHitTest(const gfx::Point& point) override;

 private:
  ~WebContentsViewHost() override;

  // The wrapper of a WebContentsViewHost is always a WebContentsView.
  static WebContentsView* AsWebContentsView(const WrapperRef<View>& api_view) {
    return static_cast<WebContentsView*>(api_view.operator->());
  }

  // NativePeer:
  void OnShutdown() override;
  void TearDownNative() override;

  // views::ViewObserver:
  void OnViewAddedToWidget(views::View* observed_view) override;
  void OnViewRemovedFromWidget(views::View* observed_view) override;
  void OnChildViewRemoved(views::View* observed_view,
                          views::View* child) override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // NativeWindowObserver:
  void UpdateWindowControlsOverlay(const gfx::Rect& bounding_rect) override;

  WebContents* GetLiveWebContents() const;
  WebContentsContainerView* container() const;
  InspectableWebContentsView* GetOwnedInspectableView() const;
  void RemoveFromParent();
  void StopObservingWindow();
  void UnregisterDraggableRegionProvider();
  void OnContentsBoundsChanging();
  bool HasLivePage();
  void ScheduleWindowControlsOverlayUpdate();
  void SendWindowControlsOverlay();

  // The wrapper holds the strong edge. This one lets the host destroy
  // the WebContents after the wrapper has been collected.
  cppgc::WeakPersistent<WebContents> api_web_contents_;
  base::WeakPtr<NativeWindow> observed_window_;
  base::WeakPtr<NativeWindow> draggable_region_window_;
  bool window_controls_overlay_update_pending_ = false;
  // Cleared at shutdown, where the WebContents disposes itself.
  bool destroy_web_contents_on_release_ = true;

  base::WeakPtrFactory<WebContentsViewHost> weak_factory_{this};
};

WebContentsViewHost::WebContentsViewHost(WebContentsView* wrapper,
                                         WebContents* web_contents)
    : View::Host(wrapper, std::make_unique<WebContentsContainerView>()),
      content::WebContentsObserver(web_contents->web_contents()),
      api_web_contents_(web_contents) {
  InspectableWebContentsView* inspectable_view =
      web_contents->inspectable_web_contents()->GetView();
  SetOwnedByClient(inspectable_view);
  container()->TakeInspectableView(inspectable_view);
  // See OnContentsBoundsChanging().
  inspectable_view->SetBoundsChangedCallback(
      base::BindRepeating(&WebContentsViewHost::OnContentsBoundsChanging,
                          weak_factory_.GetWeakPtr()));
}

WebContentsViewHost::~WebContentsViewHost() = default;

void WebContentsViewHost::OnShutdown() {
  destroy_web_contents_on_release_ = false;
}

void WebContentsViewHost::TearDownNative() {
  StopObservingWindow();
  UnregisterDraggableRegionProvider();
  window_controls_overlay_update_pending_ = false;

  InspectableWebContentsView* inspectable_view = GetOwnedInspectableView();
  if (inspectable_view)
    inspectable_view->SetBoundsChangedCallback(base::RepeatingClosure());
  cppgc::Persistent<WebContents> web_contents_to_destroy;
  if (inspectable_view && destroy_web_contents_on_release_)
    web_contents_to_destroy = GetLiveWebContents();

  weak_factory_.InvalidateWeakPtrs();
  Observe(nullptr);
  api_web_contents_.Clear();
  View::Host::TearDownNative();
  // A WebContentsView owns its webContents, unless another WebContentsView
  // has adopted it since and so taken the inspectable view into its own
  // container. Dispose now rather than in a later task, which could destroy a
  // webContents adopted in between. This runs from the peer release task,
  // never from a Chromium callback on this WebContents.
  if (web_contents_to_destroy)
    web_contents_to_destroy->DestroyNow();
}

WebContents* WebContentsViewHost::GetLiveWebContents() const {
  WebContents* web_contents = api_web_contents_.Get();
  return web_contents && !web_contents->IsDestroyed() &&
                 web_contents->web_contents()
             ? web_contents
             : nullptr;
}

WebContentsContainerView* WebContentsViewHost::container() const {
  return static_cast<WebContentsContainerView*>(view());
}

InspectableWebContentsView* WebContentsViewHost::GetOwnedInspectableView()
    const {
  return view() ? container()->GetOwnedInspectableView() : nullptr;
}

void WebContentsViewHost::RemoveFromParent() {
  if (views::View* view = this->view(); view && view->parent())
    view->parent()->RemoveChildView(view);
}

void WebContentsViewHost::RegisterDraggableRegionProvider(
    NativeWindow* window) {
  if (!is_active() || !window)
    return;
  if (draggable_region_window_.get() != window)
    UnregisterDraggableRegionProvider();
  window->AddDraggableRegionProvider(this);
  draggable_region_window_ = window->GetWeakPtr();
}

void WebContentsViewHost::UnregisterDraggableRegionProvider() {
  if (NativeWindow* window = draggable_region_window_.get())
    window->RemoveDraggableRegionProvider(this);
  draggable_region_window_ = nullptr;
}

void WebContentsViewHost::ApplyBorderRadius(std::optional<int> radius) {
  InspectableWebContentsView* inspectable_view = GetOwnedInspectableView();
  if (!radius.has_value() || !inspectable_view || !view()->GetWidget())
    return;
  inspectable_view->SetCornerRadii(gfx::RoundedCornersF(radius.value()));
}

int WebContentsViewHost::NonClientHitTest(const gfx::Point& point) {
  InspectableWebContentsView* inspectable_view = GetOwnedInspectableView();
  if (!inspectable_view || !view()->GetVisible())
    return HTNOWHERE;
  if (auto* web_contents = GetLiveWebContents()) {
    // Convert the point to the contents view's coordinate space rather than
    // the InspectableWebContentsView's coordinate space, because the draggable
    // region is relative to the web content area. When DevTools is docked
    // (e.g. to the left), the contents view is offset within the parent,
    // so we need to account for that offset.
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

void WebContentsViewHost::WebContentsDestroyed() {
  api_web_contents_.Clear();
  if (auto api_view = wrapper())
    AsWebContentsView(api_view)->OnWebContentsDestroyed();
  RemoveFromParent();
}

void WebContentsViewHost::OnChildViewRemoved(views::View* observed_view,
                                             views::View* child) {
  View::Host::OnChildViewRemoved(observed_view, child);
  if (container()->IsInspectableViewBeingDeleted(child))
    RemoveFromParent();
}

void WebContentsViewHost::OnViewAddedToWidget(views::View* observed_view) {
  DCHECK_EQ(observed_view, view());

  if (!GetOwnedInspectableView())
    return;
  NativeWindow* native_window =
      NativeWindow::FromWidget(observed_view->GetWidget());
  if (!native_window)
    return;
  WebContents* web_contents = GetLiveWebContents();
  if (!web_contents)
    return;

  // We don't need to call SetOwnerWindow(nullptr) in OnViewRemovedFromWidget
  // because that's handled in the WebContents dtor called prior.
  web_contents->SetOwnerWindow(native_window);
  RegisterDraggableRegionProvider(native_window);
  StopObservingWindow();
  observed_window_ = native_window->GetWeakPtr();
  native_window->AddObserver(this);
  if (auto api_view = wrapper())
    ApplyBorderRadius(api_view->border_radius());
  if (HasLivePage())
    ScheduleWindowControlsOverlayUpdate();
}

void WebContentsViewHost::OnViewRemovedFromWidget(views::View* observed_view) {
  DCHECK_EQ(observed_view, view());

  StopObservingWindow();
  UnregisterDraggableRegionProvider();
}

// Our bounds changed and the RenderWidgetHostView is about to be resized to
// match. Push the re-clipped overlay rect now so that it rides along with the
// resize in a single VisualProperties update, rather than trailing it (where it
// could sit behind the resize's pending ack).
void WebContentsViewHost::OnContentsBoundsChanging() {
  if (HasLivePage())
    SendWindowControlsOverlay();
}

bool WebContentsViewHost::HasLivePage() {
  // Before the first navigation there is nothing to update; the window
  // notifies us again from WebContents::DidFinishNavigation.
  return observed_window_ && web_contents() &&
         web_contents()->GetPrimaryMainFrame()->IsRenderFrameLive();
}

// NativeWindowObserver. This fires from inside the frame view's layout, before
// the client area (and so this view) has been laid out, so defer until the
// current layout pass has finished to avoid clipping against stale bounds.
void WebContentsViewHost::UpdateWindowControlsOverlay(
    const gfx::Rect& bounding_rect) {
  ScheduleWindowControlsOverlayUpdate();
}

void WebContentsViewHost::ScheduleWindowControlsOverlayUpdate() {
  if (window_controls_overlay_update_pending_)
    return;
  window_controls_overlay_update_pending_ = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&WebContentsViewHost::SendWindowControlsOverlay,
                                weak_factory_.GetWeakPtr()));
}

// The overlay rect is relative to the window's content area. Translate it
// into this view's coordinates so that views which only partially cover (or
// don't cover) the titlebar report the right env(titlebar-area-*) values.
void WebContentsViewHost::SendWindowControlsOverlay() {
  window_controls_overlay_update_pending_ = false;
  WebContents* api_web_contents = GetLiveWebContents();
  views::View* view = this->view();
  if (!api_web_contents || !observed_window_ || !GetOwnedInspectableView())
    return;
  const auto bounding_rect = observed_window_->GetWindowControlsOverlayRect();
  if (!bounding_rect)
    return;
  views::View* window_view = observed_window_->GetContentsView();
  if (!window_view || !window_view->Contains(view))
    return;

  gfx::Rect local_rect =
      views::View::ConvertRectToTarget(window_view, view, *bounding_rect);
  local_rect.Intersect(view->GetLocalBounds());
  api_web_contents->web_contents()->UpdateWindowControlsOverlay(local_rect);
}

void WebContentsViewHost::StopObservingWindow() {
  if (observed_window_)
    observed_window_->RemoveObserver(this);
  observed_window_ = nullptr;
}

const gin::WrapperInfo WebContentsView::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronView);

WebContentsView::WebContentsView(WebContents* web_contents)
    : View(DeferHost{}), api_web_contents_(web_contents) {
  SetHost(NativePeer<View>::Create<WebContentsViewHost>(this, web_contents));
}

WebContentsView::~WebContentsView() = default;

WebContentsViewHost* WebContentsView::web_contents_view_host() const {
  return static_cast<WebContentsViewHost*>(host());
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

void WebContentsView::OnWebContentsDestroyed() {
  api_web_contents_ = nullptr;
}

bool WebContentsView::IsUsable() const {
  return GetLiveWebContents();
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
  web_contents_view_host()->ApplyBorderRadius(border_radius());
}

void WebContentsView::RegisterDraggableRegionProvider(NativeWindow* window) {
  web_contents_view_host()->RegisterDraggableRegionProvider(window);
}

void WebContentsView::Trace(cppgc::Visitor* visitor) const {
  View::Trace(visitor);
  visitor->Trace(api_web_contents_);
}

const gin::WrapperInfo* WebContentsView::wrapper_info() const {
  return &kWrapperInfo;
}

const char* WebContentsView::GetHumanReadableName() const {
  return "Electron / WebContentsView";
}

// static
WebContentsView* WebContentsView::Create(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& web_preferences) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> arg = gin::DataObjectBuilder(isolate)
                                 .Set("webPreferences", web_preferences)
                                 .Build();
  v8::Local<v8::Function> constructor =
      gin_helper::Constructible<WebContentsView>::GetConstructor(isolate,
                                                                 context);
  v8::Local<v8::Object> obj;
  WebContentsView* web_contents_view = nullptr;
  if (!constructor.IsEmpty() &&
      constructor->NewInstance(context, 1, &arg).ToLocal(&obj)) {
    gin::ConvertFromV8(isolate, obj, &web_contents_view);
  }
  return web_contents_view;
}

// static
WebContentsView* WebContentsView::New(gin::Arguments* const args) {
  if (!gin_helper::ThrowIfNotConstructCall(args))
    return nullptr;
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

  auto* view = cppgc::MakeGarbageCollected<WebContentsView>(
      isolate->GetCppHeap()->GetAllocationHandle(), web_contents);
  gin_helper::BindToConstructCall(args, view);
  return view;
}

// static
void WebContentsView::FillObjectTemplate(v8::Isolate* isolate,
                                         v8::Local<v8::ObjectTemplate> templ) {
  gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod<&WebContentsView::SetBackgroundColor>("setBackgroundColor")
      .SetMethod<&WebContentsView::SetBorderRadius>("setBorderRadius")
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
  dict.Set("WebContentsView",
           gin_helper::Constructible<WebContentsView>::GetConstructor(isolate,
                                                                      context));
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_web_contents_view,
                                  Initialize)
