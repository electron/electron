// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents_view.h"

#include "base/no_destructor.h"
#include "content/public/browser/web_contents_user_data.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/common/gin_helper/constructor.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

#if defined(OS_MACOSX)
#include "shell/browser/ui/cocoa/delayed_native_view_host.h"
#endif

namespace {

// Used to indicate whether a WebContents already has a view.
class WebContentsViewRelay
    : public content::WebContentsUserData<WebContentsViewRelay> {
 public:
  ~WebContentsViewRelay() override = default;

 private:
  explicit WebContentsViewRelay(content::WebContents* contents) {}
  friend class content::WebContentsUserData<WebContentsViewRelay>;

  electron::api::WebContentsView* view_ = nullptr;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(WebContentsViewRelay);
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebContentsViewRelay)

}  // namespace

namespace electron {

namespace api {

WebContentsView::WebContentsView(v8::Isolate* isolate,
                                 gin::Handle<WebContents> web_contents,
                                 InspectableWebContents* iwc)
#if defined(OS_MACOSX)
    : View(new DelayedNativeViewHost(iwc->GetView()->GetNativeView())),
#else
    : View(iwc->GetView()->GetView()),
#endif
      web_contents_(isolate, web_contents->GetWrapper()),
      api_web_contents_(web_contents.get()) {
#if !defined(OS_MACOSX)
  // On macOS the View is a newly-created |DelayedNativeViewHost| and it is our
  // responsibility to delete it. On other platforms the View is created and
  // managed by InspectableWebContents.
  set_delete_view(false);
#endif
  WebContentsViewRelay::CreateForWebContents(web_contents->web_contents());
  Observe(web_contents->web_contents());
}

WebContentsView::~WebContentsView() {
  if (api_web_contents_) {  // destroy() is called
    // Destroy WebContents asynchronously, as we might be in GC currently and
    // WebContents emits an event in its destructor.
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce(
                       [](base::WeakPtr<WebContents> contents) {
                         if (contents)
                           contents->DestroyWebContents(
                               !Browser::Get()->is_shutting_down());
                       },
                       api_web_contents_->GetWeakPtr()));
  }
}

v8::Local<v8::Value> WebContentsView::GetWebContents(v8::Isolate* isolate) {
  if (web_contents_.IsEmpty())
    return v8::Null(isolate);
  return v8::Local<v8::Value>::New(isolate, web_contents_);
}

void WebContentsView::WebContentsDestroyed() {
  api_web_contents_ = nullptr;
  web_contents_.Reset();
}

// static
gin::Handle<WebContentsView> WebContentsView::Create(
    v8::Isolate* isolate,
    gin::Handle<WebContents> web_contents) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Function> constructor = GetConstructor(isolate);
  v8::Local<v8::Value> arg = web_contents.ToV8();
  v8::Local<v8::Object> obj;
  if (constructor->NewInstance(context, 1, &arg).ToLocal(&obj)) {
    gin::Handle<WebContentsView> web_contents_view;
    if (gin::ConvertFromV8(isolate, obj, &web_contents_view))
      return web_contents_view;
  }
  return gin::Handle<WebContentsView>();
}

// static
v8::Local<v8::Function> WebContentsView::GetConstructor(v8::Isolate* isolate) {
  static base::NoDestructor<v8::Global<v8::Function>> constructor;
  if (constructor.get()->IsEmpty()) {
    constructor->Reset(
        isolate, gin_helper::CreateConstructor<WebContentsView>(
                     isolate, base::BindRepeating(&WebContentsView::New)));
  }
  return v8::Local<v8::Function>::New(isolate, *constructor.get());
}

// static
gin_helper::WrappableBase* WebContentsView::New(
    gin_helper::Arguments* args,
    gin::Handle<WebContents> web_contents) {
  // Currently we only support InspectableWebContents, e.g. the WebContents
  // created by users directly. To support devToolsWebContents we need to create
  // a wrapper view.
  if (!web_contents->managed_web_contents()) {
    args->ThrowError("The WebContents must be created by user");
    return nullptr;
  }
  // Check if the WebContents has already been added to a view.
  if (WebContentsViewRelay::FromWebContents(web_contents->web_contents())) {
    args->ThrowError("The WebContents has already been added to a View");
    return nullptr;
  }
  // Constructor call.
  auto* view = new WebContentsView(args->isolate(), web_contents,
                                   web_contents->managed_web_contents());
  view->InitWithArgs(args);
  return view;
}

// static
void WebContentsView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "WebContentsView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("webContents", &WebContentsView::GetWebContents);
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::WebContentsView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WebContentsView", WebContentsView::GetConstructor(isolate));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_contents_view, Initialize)
