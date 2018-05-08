// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents_view.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "native_mate/dictionary.h"

#if defined(OS_MACOSX)
#include "atom/browser/ui/cocoa/delayed_native_view_host.h"
#endif

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

WebContentsView::WebContentsView(
    v8::Isolate* isolate,
    v8::Local<v8::Value> web_contents_wrapper,
    brightray::InspectableWebContents* web_contents)
#if defined(OS_MACOSX)
    : View(new DelayedNativeViewHost(web_contents->GetView()->GetNativeView())),
#else
    : View(web_contents->GetView()->GetView()),
#endif
      web_contents_wrapper_(isolate, web_contents_wrapper) {
#if defined(OS_MACOSX)
  // On macOS a View is created to wrap the NSView, and its lifetime is managed
  // by us.
  view()->set_owned_by_client();
#else
  // On other platforms the View is managed by InspectableWebContents.
  set_delete_view(false);
#endif
}

WebContentsView::~WebContentsView() {}

// static
mate::WrappableBase* WebContentsView::New(
    v8::Isolate* isolate,
    mate::Handle<WebContents> web_contents) {
  if (!web_contents->managed_web_contents()) {
    const char* error = "The WebContents must be created by user";
    isolate->ThrowException(
        v8::Exception::Error(mate::StringToV8(isolate, error)));
    return nullptr;
  }
  return new WebContentsView(isolate, web_contents->GetWrapper(),
                             web_contents->managed_web_contents());
}

// static
void WebContentsView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WebContentsView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  WebContentsView::SetConstructor(isolate, base::Bind(&WebContentsView::New));

  mate::Dictionary constructor(
      isolate, WebContentsView::GetConstructor(isolate)->GetFunction());

  mate::Dictionary dict(isolate, exports);
  dict.Set("WebContentsView", constructor);
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_web_contents_view, Initialize)
