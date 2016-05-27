// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_render_process_preferences.h"

#include "atom/browser/atom_browser_client.h"
#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

namespace atom {

namespace api {

namespace {

bool IsBrowserWindow(content::RenderProcessHost* process) {
  content::WebContents* web_contents =
      static_cast<AtomBrowserClient*>(AtomBrowserClient::Get())->
          GetWebContentsFromProcessID(process->GetID());
  if (!web_contents)
    return false;

  NativeWindow* window = NativeWindow::FromWebContents(web_contents);
  if (!window)
    return false;

  return true;
}

}  // namespace

RenderProcessPreferences::RenderProcessPreferences(
    v8::Isolate* isolate,
    const atom::RenderProcessPreferences::Predicate& predicate)
    : preferences_(predicate) {
  Init(isolate);
}

RenderProcessPreferences::~RenderProcessPreferences() {
}

int RenderProcessPreferences::AddEntry(const base::DictionaryValue& entry) {
  return preferences_.AddEntry(entry);
}

void RenderProcessPreferences::RemoveEntry(int id) {
  preferences_.RemoveEntry(id);
}

// static
void RenderProcessPreferences::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::ObjectTemplate> prototype) {
  mate::ObjectTemplateBuilder(isolate, prototype)
      .SetMethod("addEntry", &RenderProcessPreferences::AddEntry)
      .SetMethod("removeEntry", &RenderProcessPreferences::RemoveEntry);
}

// static
mate::Handle<RenderProcessPreferences>
RenderProcessPreferences::ForAllBrowserWindow(v8::Isolate* isolate) {
  return mate::CreateHandle(
      isolate,
      new RenderProcessPreferences(isolate, base::Bind(&IsBrowserWindow)));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("forAllBrowserWindow",
                 &atom::api::RenderProcessPreferences::ForAllBrowserWindow);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_render_process_preferences,
                                  Initialize)
