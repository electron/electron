// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_browser_view.h"

#include <vector>

#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_browser_view.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/common/color_util.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "ui/gfx/geometry/rect.h"

namespace gin {

template <>
struct Converter<electron::AutoResizeFlags> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::AutoResizeFlags* auto_resize_flags) {
    gin_helper::Dictionary params;
    if (!ConvertFromV8(isolate, val, &params)) {
      return false;
    }

    uint8_t flags = 0;
    bool width = false;
    if (params.Get("width", &width) && width) {
      flags |= electron::kAutoResizeWidth;
    }
    bool height = false;
    if (params.Get("height", &height) && height) {
      flags |= electron::kAutoResizeHeight;
    }
    bool horizontal = false;
    if (params.Get("horizontal", &horizontal) && horizontal) {
      flags |= electron::kAutoResizeHorizontal;
    }
    bool vertical = false;
    if (params.Get("vertical", &vertical) && vertical) {
      flags |= electron::kAutoResizeVertical;
    }

    *auto_resize_flags = static_cast<electron::AutoResizeFlags>(flags);
    return true;
  }
};

}  // namespace gin

namespace {

int32_t GetNextId() {
  static int32_t next_id = 1;
  return next_id++;
}

}  // namespace

namespace electron {

namespace api {

gin::WrapperInfo BrowserView::kWrapperInfo = {gin::kEmbedderNativeGin};

BrowserView::BrowserView(gin::Arguments* args,
                         const gin_helper::Dictionary& options)
    : id_(GetNextId()) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Dictionary web_preferences =
      gin::Dictionary::CreateEmpty(isolate);
  options.Get(options::kWebPreferences, &web_preferences);
  web_preferences.Set("type", "browserView");

  v8::Local<v8::Value> value;

  // Copy the webContents option to webPreferences. This is only used internally
  // to implement nativeWindowOpen option.
  if (options.Get("webContents", &value)) {
    web_preferences.SetHidden("webContents", value);
  }

  auto web_contents =
      WebContents::CreateFromWebPreferences(args->isolate(), web_preferences);

  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents.get();
  api_web_contents_->AddObserver(this);
  Observe(web_contents->web_contents());

  view_.reset(
      NativeBrowserView::Create(api_web_contents_->inspectable_web_contents()));
}

void BrowserView::SetOwnerWindow(NativeWindow* window) {
  // Ensure WebContents and BrowserView owner windows are in sync.
  if (web_contents())
    web_contents()->SetOwnerWindow(window);

  owner_window_ = window ? window->GetWeakPtr() : nullptr;
}

BrowserView::~BrowserView() {
  if (web_contents()) {  // destroy() called without closing WebContents
    web_contents()->RemoveObserver(this);
    web_contents()->Destroy();
  }
}

void BrowserView::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
  web_contents_.Reset();
  Unpin();
}

void BrowserView::OnDraggableRegionsUpdated(
    const std::vector<mojom::DraggableRegionPtr>& regions) {
  view_->UpdateDraggableRegions(regions);
}

// static
gin::Handle<BrowserView> BrowserView::New(gin_helper::ErrorThrower thrower,
                                          gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create BrowserView before app is ready");
    return gin::Handle<BrowserView>();
  }

  gin::Dictionary options = gin::Dictionary::CreateEmpty(args->isolate());
  args->GetNext(&options);

  auto handle =
      gin::CreateHandle(args->isolate(), new BrowserView(args, options));
  handle->Pin(args->isolate());
  return handle;
}

void BrowserView::SetAutoResize(AutoResizeFlags flags) {
  view_->SetAutoResizeFlags(flags);
}

void BrowserView::SetBounds(const gfx::Rect& bounds) {
  view_->SetBounds(bounds);
}

gfx::Rect BrowserView::GetBounds() {
  return view_->GetBounds();
}

void BrowserView::SetBackgroundColor(const std::string& color_name) {
  view_->SetBackgroundColor(ParseHexColor(color_name));
}

v8::Local<v8::Value> BrowserView::GetWebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty()) {
    return v8::Null(isolate);
  }

  return v8::Local<v8::Value>::New(isolate, web_contents_);
}

// static
v8::Local<v8::ObjectTemplate> BrowserView::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  return gin::ObjectTemplateBuilder(isolate, "BrowserView", templ)
      .SetMethod("setAutoResize", &BrowserView::SetAutoResize)
      .SetMethod("setBounds", &BrowserView::SetBounds)
      .SetMethod("getBounds", &BrowserView::GetBounds)
      .SetMethod("setBackgroundColor", &BrowserView::SetBackgroundColor)
      .SetProperty("webContents", &BrowserView::GetWebContents)
      .Build();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::BrowserView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("BrowserView", BrowserView::GetConstructor(context));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_browser_view, Initialize)
