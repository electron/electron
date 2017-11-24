// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_browser_view.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_browser_view.h"
#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "ui/gfx/geometry/rect.h"

namespace mate {

template <>
struct Converter<atom::AutoResizeFlags> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::AutoResizeFlags* auto_resize_flags) {
    mate::Dictionary params;
    if (!ConvertFromV8(isolate, val, &params)) {
      return false;
    }

    uint8_t flags = 0;
    bool width = false;
    if (params.Get("width", &width) && width) {
      flags |= atom::kAutoResizeWidth;
    }
    bool height = false;
    if (params.Get("height", &height) && height) {
      flags |= atom::kAutoResizeHeight;
    }

    *auto_resize_flags = static_cast<atom::AutoResizeFlags>(flags);
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

BrowserView::BrowserView(v8::Isolate* isolate,
                         v8::Local<v8::Object> wrapper,
                         const mate::Dictionary& options)
    : api_web_contents_(nullptr) {
  Init(isolate, wrapper, options);
}

void BrowserView::Init(v8::Isolate* isolate,
                       v8::Local<v8::Object> wrapper,
                       const mate::Dictionary& options) {
  mate::Dictionary web_preferences = mate::Dictionary::CreateEmpty(isolate);
  options.Get(options::kWebPreferences, &web_preferences);
  web_preferences.Set("isBrowserView", true);
  mate::Handle<class WebContents> web_contents =
      WebContents::Create(isolate, web_preferences);

  web_contents_.Reset(isolate, web_contents.ToV8());
  api_web_contents_ = web_contents.get();

  view_.reset(NativeBrowserView::Create(
      api_web_contents_->managed_web_contents()->GetView()));

  InitWith(isolate, wrapper);
}

BrowserView::~BrowserView() {
  api_web_contents_->DestroyWebContents(true /* async */);
}

// static
mate::WrappableBase* BrowserView::New(mate::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    args->ThrowError("Cannot create BrowserView before app is ready");
    return nullptr;
  }

  if (args->Length() > 1) {
    args->ThrowError("Too many arguments");
    return nullptr;
  }

  mate::Dictionary options;
  if (!(args->Length() == 1 && args->GetNext(&options))) {
    options = mate::Dictionary::CreateEmpty(args->isolate());
  }

  return new BrowserView(args->isolate(), args->GetThis(), options);
}

int32_t BrowserView::ID() const {
  return weak_map_id();
}

void BrowserView::SetAutoResize(AutoResizeFlags flags) {
  view_->SetAutoResizeFlags(flags);
}

void BrowserView::SetBounds(const gfx::Rect& bounds) {
  view_->SetBounds(bounds);
}

void BrowserView::SetBackgroundColor(const std::string& color_name) {
  view_->SetBackgroundColor(ParseHexColor(color_name));
}

v8::Local<v8::Value> BrowserView::GetWebContents() {
  if (web_contents_.IsEmpty()) {
    return v8::Null(isolate());
  }

  return v8::Local<v8::Value>::New(isolate(), web_contents_);
}

// static
void BrowserView::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "BrowserView"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("setAutoResize", &BrowserView::SetAutoResize)
      .SetMethod("setBounds", &BrowserView::SetBounds)
      .SetMethod("setBackgroundColor", &BrowserView::SetBackgroundColor)
      .SetProperty("webContents", &BrowserView::GetWebContents)
      .SetProperty("id", &BrowserView::ID);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::BrowserView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  BrowserView::SetConstructor(isolate, base::Bind(&BrowserView::New));

  mate::Dictionary browser_view(
      isolate, BrowserView::GetConstructor(isolate)->GetFunction());
  browser_view.SetMethod("fromId",
                          &mate::TrackableObject<BrowserView>::FromWeakMapID);
  browser_view.SetMethod("getAllViews",
                          &mate::TrackableObject<BrowserView>::GetAll);
  mate::Dictionary dict(isolate, exports);
  dict.Set("BrowserView", browser_view);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_browser_view, Initialize)
