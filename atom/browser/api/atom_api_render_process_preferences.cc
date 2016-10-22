// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_render_process_preferences.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

namespace {

bool IsWebContents(v8::Isolate* isolate, content::RenderProcessHost* process) {
  content::WebContents* web_contents =
      static_cast<AtomBrowserClient*>(AtomBrowserClient::Get())->
          GetWebContentsFromProcessID(process->GetID());
  if (!web_contents)
    return false;

  auto api_web_contents = WebContents::CreateFrom(isolate, web_contents);
  auto type = api_web_contents->GetType();
  return type == WebContents::Type::BROWSER_WINDOW ||
         type == WebContents::Type::WEB_VIEW;
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
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(
      mate::StringToV8(isolate, "RenderProcessPreferences"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("addEntry", &RenderProcessPreferences::AddEntry)
      .SetMethod("removeEntry", &RenderProcessPreferences::RemoveEntry);
}

// static
mate::Handle<RenderProcessPreferences>
RenderProcessPreferences::ForAllWebContents(v8::Isolate* isolate) {
  return mate::CreateHandle(
      isolate,
      new RenderProcessPreferences(isolate,
                                   base::Bind(&IsWebContents, isolate)));
}

}  // namespace api

}  // namespace atom

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("forAllWebContents",
                 &atom::api::RenderProcessPreferences::ForAllWebContents);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_render_process_preferences,
                                  Initialize)
