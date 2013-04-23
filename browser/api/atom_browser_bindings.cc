// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_browser_bindings.h"

#include <vector>

#include "base/logging.h"
#include "base/values.h"
#include "common/v8_value_converter_impl.h"
#include "content/public/browser/browser_thread.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

using content::V8ValueConverter;
using node::node_isolate;

namespace atom {

AtomBrowserBindings::AtomBrowserBindings() {
}

AtomBrowserBindings::~AtomBrowserBindings() {
}

void AtomBrowserBindings::AfterLoad() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  v8::HandleScope scope;

  v8::Handle<v8::Object> global = node::g_context->Global();
  v8::Handle<v8::Object> atom =
      global->Get(v8::String::New("__atom"))->ToObject();
  DCHECK(!atom.IsEmpty());

  browser_main_parts_ = v8::Persistent<v8::Object>::New(
      node_isolate,
      atom->Get(v8::String::New("browserMainParts"))->ToObject());
  DCHECK(!browser_main_parts_.IsEmpty());
}

void AtomBrowserBindings::OnRendererMessage(int process_id,
                                            int routing_id,
                                            const std::string& channel,
                                            const base::ListValue& args) {
  v8::HandleScope scope;

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());

  // process.emit(channel, 'message', process_id, routing_id);
  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(3 + args.GetSize());
  arguments.push_back(v8::String::New(channel.c_str(), channel.size()));
  const base::Value* value;
  if (args.Get(0, &value))
    arguments.push_back(converter->ToV8Value(value, context));
  arguments.push_back(v8::Integer::New(process_id));
  arguments.push_back(v8::Integer::New(routing_id));

  for (size_t i = 1; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, context));
  }

  node::MakeCallback(node::process, "emit", arguments.size(), &arguments[0]);
}

void AtomBrowserBindings::OnRendererMessageSync(
    int process_id,
    int routing_id,
    const std::string& channel,
    const base::ListValue& args,
    base::DictionaryValue* result) {
  v8::HandleScope scope;

  v8::Handle<v8::Context> context = v8::Context::GetCurrent();

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverterImpl());

  v8::Handle<v8::Object> event = v8::Object::New();

  // process.emit(channel, 'sync-message', event, process_id, routing_id);
  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(3 + args.GetSize());
  arguments.push_back(v8::String::New(channel.c_str(), channel.size()));
  const base::Value* value;
  if (args.Get(0, &value))
    arguments.push_back(converter->ToV8Value(value, context));
  arguments.push_back(event);
  arguments.push_back(v8::Integer::New(process_id));
  arguments.push_back(v8::Integer::New(routing_id));

  for (size_t i = 1; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, context));
  }

  node::MakeCallback(node::process, "emit", arguments.size(), &arguments[0]);

  scoped_ptr<base::Value> base_event(converter->FromV8Value(event, context));
  DCHECK(base_event && base_event->IsType(base::Value::TYPE_DICTIONARY));

  result->Swap(static_cast<base::DictionaryValue*>(base_event.get()));
}

}  // namespace atom
