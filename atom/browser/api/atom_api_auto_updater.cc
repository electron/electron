// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_auto_updater.h"

#include "base/time/time.h"
#include "atom/browser/browser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/node_includes.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

namespace mate {

template<>
struct Converter<base::Time> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Time& val) {
    v8::MaybeLocal<v8::Value> date = v8::Date::New(
        isolate->GetCurrentContext(), val.ToJsTime());
    if (date.IsEmpty())
      return v8::Null(isolate);
    else
      return date.ToLocalChecked();
  }
};

}  // namespace mate

namespace atom {

namespace api {

AutoUpdater::AutoUpdater(v8::Isolate* isolate) {
  auto_updater::AutoUpdater::SetDelegate(this);
  Init(isolate);
}

AutoUpdater::~AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(nullptr);
}

void AutoUpdater::OnError(const std::string& message) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  auto error = v8::Exception::Error(mate::StringToV8(isolate(), message));
  EmitCustomEvent(
      "error",
      error->ToObject(isolate()->GetCurrentContext()).ToLocalChecked(),
      // Message is also emitted to keep compatibility with old code.
      message);
}

void AutoUpdater::OnCheckingForUpdate() {
  Emit("checking-for-update");
}

void AutoUpdater::OnUpdateAvailable() {
  Emit("update-available");
}

void AutoUpdater::OnUpdateNotAvailable() {
  Emit("update-not-available");
}

void AutoUpdater::OnUpdateDownloaded(const std::string& release_notes,
                                     const std::string& release_name,
                                     const base::Time& release_date,
                                     const std::string& url) {
  Emit("update-downloaded", release_notes, release_name, release_date, url,
       // Keep compatibility with old APIs.
       base::Bind(&AutoUpdater::QuitAndInstall, base::Unretained(this)));
}

void AutoUpdater::OnWindowAllClosed() {
  QuitAndInstall();
}

void AutoUpdater::SetFeedURL(const std::string& url, mate::Arguments* args) {
  auto_updater::AutoUpdater::HeaderMap headers;
  args->GetNext(&headers);
  auto_updater::AutoUpdater::SetFeedURL(url, headers);
}

void AutoUpdater::QuitAndInstall() {
  // If we don't have any window then quitAndInstall immediately.
  WindowList* window_list = WindowList::GetInstance();
  if (window_list->size() == 0) {
    auto_updater::AutoUpdater::QuitAndInstall();
    return;
  }

  // Otherwise do the restart after all windows have been closed.
  window_list->AddObserver(this);
  for (NativeWindow* window : *window_list)
    window->Close();
}

// static
mate::Handle<AutoUpdater> AutoUpdater::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new AutoUpdater(isolate));
}

// static
void AutoUpdater::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "AutoUpdater"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("checkForUpdates", &auto_updater::AutoUpdater::CheckForUpdates)
      .SetMethod("getFeedURL", &auto_updater::AutoUpdater::GetFeedURL)
      .SetMethod("setFeedURL", &AutoUpdater::SetFeedURL)
      .SetMethod("quitAndInstall", &AutoUpdater::QuitAndInstall);
}

}  // namespace api

}  // namespace atom


namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("autoUpdater", atom::api::AutoUpdater::Create(isolate));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_auto_updater, Initialize)
