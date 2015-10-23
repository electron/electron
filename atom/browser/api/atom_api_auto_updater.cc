// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_auto_updater.h"

#include "base/time/time.h"
#include "atom/browser/auto_updater.h"
#include "atom/browser/browser.h"
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

AutoUpdater::AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(this);
}

AutoUpdater::~AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(NULL);
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
                                     const std::string& url,
                                     const base::Closure& quit_and_install) {
  quit_and_install_ = quit_and_install;
  Emit("update-downloaded", release_notes, release_name, release_date, url);
}

mate::ObjectTemplateBuilder AutoUpdater::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("setFeedUrl", &auto_updater::AutoUpdater::SetFeedURL)
      .SetMethod("checkForUpdates", &auto_updater::AutoUpdater::CheckForUpdates)
      .SetMethod("_quitAndInstall", &AutoUpdater::QuitAndInstall);
}

void AutoUpdater::QuitAndInstall() {
  if (quit_and_install_.is_null())
    Browser::Get()->Shutdown();
  else
    quit_and_install_.Run();
}

// static
mate::Handle<AutoUpdater> AutoUpdater::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new AutoUpdater);
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
