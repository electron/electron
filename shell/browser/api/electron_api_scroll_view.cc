#include "shell/browser/api/electron_api_scroll_view.h"

#include "gin/handle.h"
#include "shell/browser/api/electron_api_base_view.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

namespace {

ScrollBarMode ConvertToScrollBarMode(std::string mode) {
  if (mode == "disabled")
    return ScrollBarMode::kDisabled;
  else if (mode == "hidden-but-enabled")
    return ScrollBarMode::kHiddenButEnabled;
  return ScrollBarMode::kEnabled;
}

std::string ConvertFromScrollBarMode(ScrollBarMode mode) {
  if (mode == ScrollBarMode::kDisabled)
    return "disabled";
  else if (mode == ScrollBarMode::kHiddenButEnabled)
    return "hidden-but-enabled";
  return "enabled";
}

#if defined(OS_MAC)
ScrollElasticity ConvertToScrollElasticity(std::string elasticity) {
  if (elasticity == "none")
    return ScrollElasticity::kNone;
  else if (elasticity == "allowed")
    return ScrollElasticity::kAllowed;
  return ScrollElasticity::kAutomatic;
}

std::string ConvertFromScrollElasticity(ScrollElasticity elasticity) {
  if (elasticity == ScrollElasticity::kNone)
    return "none";
  else if (elasticity == ScrollElasticity::kAllowed)
    return "allowed";
  return "automatic";
}
#endif

}  // namespace

ScrollView::ScrollView(gin::Arguments* args, NativeScrollView* scroll)
    : BaseView(args->isolate(), scroll), scroll_(scroll) {
  InitWithArgs(args);
}

ScrollView::~ScrollView() = default;

void ScrollView::ResetChildView(BaseView* view) {
  if (view->GetID() == content_view_id_) {
    content_view_id_ = 0;
    content_view_.Reset();
  }
}

void ScrollView::ResetChildViews() {
  content_view_id_ = 0;
  content_view_.Reset();
}

void ScrollView::SetContentView(v8::Local<v8::Value> value) {
  gin::Handle<BaseView> content_view;
  if (value->IsObject() &&
      gin::ConvertFromV8(isolate(), value, &content_view)) {
    if (content_view->GetID() != content_view_id_) {
      if (!content_view->EnsureDetachFromParent())
        return;
      scroll_->SetContentView(content_view->view());
      content_view_id_ = content_view->GetID();
      content_view_.Reset(isolate(), value);
    }
  }
}

v8::Local<v8::Value> ScrollView::GetContentView() const {
  if (content_view_.IsEmpty())
    return v8::Null(isolate());

  return v8::Local<v8::Value>::New(isolate(), content_view_);
}

void ScrollView::SetContentSize(gfx::Size size) {
  scroll_->SetContentSize(size);
}

gfx::Size ScrollView::GetContentSize() const {
  return scroll_->GetContentSize();
}

void ScrollView::SetHorizontalScrollBarMode(std::string mode) {
  scroll_->SetHorizontalScrollBarMode(ConvertToScrollBarMode(mode));
}

std::string ScrollView::GetHorizontalScrollBarMode() const {
  return ConvertFromScrollBarMode(scroll_->GetHorizontalScrollBarMode());
}

void ScrollView::SetVerticalScrollBarMode(std::string mode) {
  scroll_->SetVerticalScrollBarMode(ConvertToScrollBarMode(mode));
}

std::string ScrollView::GetVerticalScrollBarMode() const {
  return ConvertFromScrollBarMode(scroll_->GetVerticalScrollBarMode());
}

#if defined(OS_MAC)
void ScrollView::SetHorizontalScrollElasticity(std::string elasticity) {
  scroll_->SetHorizontalScrollElasticity(ConvertToScrollElasticity(elasticity));
}

std::string ScrollView::GetHorizontalScrollElasticity() const {
  return ConvertFromScrollElasticity(scroll_->GetHorizontalScrollElasticity());
}

void ScrollView::SetVerticalScrollElasticity(std::string elasticity) {
  scroll_->SetVerticalScrollElasticity(ConvertToScrollElasticity(elasticity));
}

std::string ScrollView::GetVerticalScrollElasticity() const {
  return ConvertFromScrollElasticity(scroll_->GetVerticalScrollElasticity());
}

void ScrollView::SetScrollPosition(gfx::Point point) {
  scroll_->SetScrollPosition(point);
}

gfx::Point ScrollView::GetScrollPosition() const {
  return scroll_->GetScrollPosition();
}

gfx::Point ScrollView::GetMaximumScrollPosition() const {
  return scroll_->GetMaximumScrollPosition();
}

void ScrollView::SetOverlayScrollbar(bool overlay) {
  scroll_->SetOverlayScrollbar(overlay);
}

bool ScrollView::IsOverlayScrollbar() const {
  return scroll_->IsOverlayScrollbar();
}
#endif

#if defined(TOOLKIT_VIEWS) && !defined(OS_MAC)
void ScrollView::ClipHeightTo(int min_height, int max_height) {
  scroll_->ClipHeightTo(min_height, max_height);
}

int ScrollView::GetMinHeight() const {
  return scroll_->GetMinHeight();
}

int ScrollView::GetMaxHeight() const {
  return scroll_->GetMaxHeight();
}

gfx::Rect ScrollView::GetVisibleRect() const {
  return scroll_->GetVisibleRect();
}

void ScrollView::SetAllowKeyboardScrolling(bool allow) {
  scroll_->SetAllowKeyboardScrolling(allow);
}

bool ScrollView::GetAllowKeyboardScrolling() const {
  return scroll_->GetAllowKeyboardScrolling();
}

void ScrollView::SetDrawOverflowIndicator(bool indicator) {
  scroll_->SetDrawOverflowIndicator(indicator);
}

bool ScrollView::GetDrawOverflowIndicator() const {
  return scroll_->GetDrawOverflowIndicator();
}
#endif

// static
gin_helper::WrappableBase* ScrollView::New(gin_helper::ErrorThrower thrower,
                                           gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create ScrollView before app is ready");
    return nullptr;
  }

  return new ScrollView(args, new NativeScrollView());
}

// static
void ScrollView::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "ScrollView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setContentView", &ScrollView::SetContentView)
      .SetMethod("getContentView", &ScrollView::GetContentView)
      .SetMethod("setContentSize", &ScrollView::SetContentSize)
      .SetMethod("getContentSize", &ScrollView::GetContentSize)
      .SetMethod("setHorizontalScrollBarMode",
                 &ScrollView::SetHorizontalScrollBarMode)
      .SetMethod("getHorizontalScrollBarMode",
                 &ScrollView::GetHorizontalScrollBarMode)
      .SetMethod("setVerticalScrollBarMode",
                 &ScrollView::SetVerticalScrollBarMode)
      .SetMethod("getVerticalScrollBarMode",
                 &ScrollView::GetVerticalScrollBarMode)
#if defined(OS_MAC)
      .SetMethod("setHorizontalScrollElasticity",
                 &ScrollView::SetHorizontalScrollElasticity)
      .SetMethod("getHorizontalScrollElasticity",
                 &ScrollView::GetHorizontalScrollElasticity)
      .SetMethod("setVerticalScrollElasticity",
                 &ScrollView::SetVerticalScrollElasticity)
      .SetMethod("getVerticalScrollElasticity",
                 &ScrollView::GetVerticalScrollElasticity)
      .SetMethod("setScrollPosition", &ScrollView::SetScrollPosition)
      .SetMethod("getScrollPosition", &ScrollView::GetScrollPosition)
      .SetMethod("getMaximumScrollPosition",
                 &ScrollView::GetMaximumScrollPosition)
      .SetMethod("setOverlayScrollbar", &ScrollView::SetOverlayScrollbar)
      .SetMethod("isOverlayScrollbar", &ScrollView::IsOverlayScrollbar)
#endif
#if defined(TOOLKIT_VIEWS) && !defined(OS_MAC)
      .SetMethod("clipHeightTo", &ScrollView::ClipHeightTo)
      .SetMethod("getMinHeight", &ScrollView::GetMinHeight)
      .SetMethod("getMaxHeight", &ScrollView::GetMaxHeight)
      .SetMethod("getVisibleRect", &ScrollView::GetVisibleRect)
      .SetMethod("setAllowKeyboardScrolling",
                 &ScrollView::SetAllowKeyboardScrolling)
      .SetMethod("getAllowKeyboardScrolling",
                 &ScrollView::GetAllowKeyboardScrolling)
      .SetMethod("setDrawOverflowIndicator",
                 &ScrollView::SetDrawOverflowIndicator)
      .SetMethod("getDrawOverflowIndicator",
                 &ScrollView::GetDrawOverflowIndicator)
#endif
      .Build();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::ScrollView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("ScrollView", gin_helper::CreateConstructor<ScrollView>(
                             isolate, base::BindRepeating(&ScrollView::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_scroll_view, Initialize)
