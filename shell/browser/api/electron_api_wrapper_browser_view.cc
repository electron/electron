#include "shell/browser/api/electron_api_wrapper_browser_view.h"

#include "gin/handle.h"
#include "shell/browser/api/electron_api_browser_view.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/browser.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/node_includes.h"

namespace electron {

namespace api {

WrapperBrowserView::WrapperBrowserView(gin::Arguments* args,
                                       const gin_helper::Dictionary& options,
                                       NativeWrapperBrowserView* view)
    : BaseView(args->isolate(), view), view_(view) {
  InitWithArgs(args);

  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Value> value;
  gin::Handle<BrowserView> browser_view;
  if (options.Get("browserView", &value) && value->IsObject() &&
      gin::ConvertFromV8(isolate, value, &browser_view)) {
    // If we're reparenting a BrowserView, ensure that it's detached from
    // its previous owner window/view.
    auto* owner_window = browser_view->owner_window();
    auto* owner_view = browser_view->owner_view();
    if (owner_view) {
      owner_view->DetachBrowserView(browser_view->view());
      browser_view->SetOwnerView(nullptr);
    } else if (owner_window) {
      owner_window->RemoveBrowserView(browser_view->view());
      browser_view->SetOwnerWindow(nullptr);
    }
    browser_view_.Reset(isolate, value);
  } else {
    gin::Dictionary options = gin::Dictionary::CreateEmpty(isolate);
    browser_view = gin::CreateHandle(isolate, new BrowserView(args, options));
    browser_view->Pin(isolate);
    browser_view_.Reset(isolate, browser_view.ToV8());
  }

  api_browser_view_ = browser_view.get();
  view->SetBrowserView(api_browser_view_);
  api_browser_view_->SetOwnerView(view);

  if (browser_view->web_contents())
    Observe(browser_view->web_contents()->web_contents());
}

WrapperBrowserView::~WrapperBrowserView() {
  if (api_browser_view_) {
    view_->DetachBrowserView(api_browser_view_->view());
    api_browser_view_->SetOwnerView(nullptr);
  }
  view_->SetBrowserView(nullptr);
}

void WrapperBrowserView::WebContentsDestroyed() {
  view_->SetBrowserView(nullptr);
  api_browser_view_ = nullptr;
  browser_view_.Reset();
}

v8::Local<v8::Value> WrapperBrowserView::GetBrowserView(v8::Isolate* isolate) {
  if (browser_view_.IsEmpty())
    return v8::Null(isolate);
  return v8::Local<v8::Value>::New(isolate, browser_view_);
}

// static
gin_helper::WrappableBase* WrapperBrowserView::New(
    gin_helper::ErrorThrower thrower,
    gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create WrapperBrowserView before app is ready");
    return nullptr;
  }

  gin::Dictionary options = gin::Dictionary::CreateEmpty(args->isolate());
  args->GetNext(&options);

  return new WrapperBrowserView(args, options, new NativeWrapperBrowserView());
}

// static
void WrapperBrowserView::BuildPrototype(
    v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(gin::StringToV8(isolate, "WrapperBrowserView"));
  gin_helper::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("browserView", &WrapperBrowserView::GetBrowserView)
      .Build();
}

}  // namespace api

}  // namespace electron

namespace {

using electron::api::WrapperBrowserView;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WrapperBrowserView",
           gin_helper::CreateConstructor<WrapperBrowserView>(
               isolate, base::BindRepeating(&WrapperBrowserView::New)));
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_wrapper_browser_view,
                                 Initialize)
