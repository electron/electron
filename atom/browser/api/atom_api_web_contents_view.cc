// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_contents_view.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/common/api/constructor.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "content/public/browser/web_contents_user_data.h"
#include "native_mate/dictionary.h"

#if defined(OS_MACOSX)
#include "atom/browser/ui/cocoa/delayed_native_view_host.h"
#endif

#include "atom/common/node_includes.h"

namespace {

// Used to indicate whether a WebContents already has a view.
class WebContentsViewRelay
    : public content::WebContentsUserData<WebContentsViewRelay> {
 public:
  ~WebContentsViewRelay() override {}

 private:
  explicit WebContentsViewRelay(content::WebContents* contents) {}
  friend class content::WebContentsUserData<WebContentsViewRelay>;

  atom::api::WebContentsView* view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewRelay);
};

}  // namespace

DEFINE_WEB_CONTENTS_USER_DATA_KEY(WebContentsViewRelay);

namespace atom {

namespace api {

WebContentsView::WebContentsView(v8::Isolate* isolate,
                                 mate::Handle<WebContents> web_contents,
                                 brightray::InspectableWebContents* iwc)
#if defined(OS_MACOSX)
    : View(new DelayedNativeViewHost(iwc->GetView()->GetNativeView())),
#else
    : View(iwc->GetView()->GetView()),
#endif
      web_contents_(isolate, web_contents->GetWrapper()),
      api_web_contents_(web_contents.get()) {
#if defined(OS_MACOSX)
  // On macOS a View is created to wrap the NSView, and its lifetime is managed
  // by us.
  view()->set_owned_by_client();
#else
  // On other platforms the View is managed by InspectableWebContents.
  set_delete_view(false);
#endif
  WebContentsViewRelay::CreateForWebContents(web_contents->web_contents());
  Observe(web_contents->web_contents());
}

WebContentsView::~WebContentsView() {
  if (api_web_contents_)
    api_web_contents_->DestroyWebContents(false /* async */);
}

void WebContentsView::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
  web_contents_.Reset();
}

// static
mate::WrappableBase* WebContentsView::New(
    mate::Arguments* args,
    mate::Handle<WebContents> web_contents) {
  // Currently we only support InspectableWebContents, e.g. the WebContents
  // created by users directly. To support devToolsWebContents we need to create
  // a wrapper view.
  if (!web_contents->managed_web_contents()) {
    const char* error = "The WebContents must be created by user";
    args->isolate()->ThrowException(
        v8::Exception::Error(mate::StringToV8(args->isolate(), error)));
    return nullptr;
  }
  // Check if the WebContents has already been added to a view.
  if (WebContentsViewRelay::FromWebContents(web_contents->web_contents())) {
    const char* error = "The WebContents has already been added to a View";
    args->isolate()->ThrowException(
        v8::Exception::Error(mate::StringToV8(args->isolate(), error)));
    return nullptr;
  }
  // Constructor call.
  auto* view = new WebContentsView(args->isolate(), web_contents,
                                   web_contents->managed_web_contents());
  view->InitWith(args->isolate(), args->GetThis());
  return view;
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
  mate::Dictionary dict(isolate, exports);
  dict.Set("WebContentsView", mate::CreateConstructor<WebContentsView>(
                                  isolate, base::Bind(&WebContentsView::New)));
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_web_contents_view, Initialize)
