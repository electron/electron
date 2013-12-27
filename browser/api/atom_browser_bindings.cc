// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_browser_bindings.h"

#include <vector>

#include "base/logging.h"
#include "browser/api/atom_api_event.h"
#include "common/v8/native_type_conversions.h"
#include "content/public/browser/browser_thread.h"

#include "common/v8/node_common.h"

namespace atom {

AtomBrowserBindings::AtomBrowserBindings() {
}

AtomBrowserBindings::~AtomBrowserBindings() {
}

void AtomBrowserBindings::OnRendererMessage(int process_id,
                                            int routing_id,
                                            const string16& channel,
                                            const base::ListValue& args) {
  v8::HandleScope handle_scope(node_isolate);

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverter);

  // process.emit(channel, 'message', process_id, routing_id);
  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(3 + args.GetSize());
  arguments.push_back(ToV8Value(channel));
  const base::Value* value;
  if (args.Get(0, &value))
    arguments.push_back(converter->ToV8Value(value, global_env->context()));
  arguments.push_back(v8::Integer::New(process_id));
  arguments.push_back(v8::Integer::New(routing_id));

  for (size_t i = 1; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, global_env->context()));
  }

  node::MakeCallback(global_env->process_object(),
                     "emit",
                     arguments.size(),
                     &arguments[0]);
}

void AtomBrowserBindings::OnRendererMessageSync(
    int process_id,
    int routing_id,
    const string16& channel,
    const base::ListValue& args,
    NativeWindow* sender,
    IPC::Message* message) {
  v8::HandleScope handle_scope(node_isolate);

  scoped_ptr<V8ValueConverter> converter(new V8ValueConverter);

  // Create the event object.
  v8::Handle<v8::Object> event = api::Event::CreateV8Object();
  api::Event::Unwrap<api::Event>(event)->SetSenderAndMessage(sender, message);

  // process.emit(channel, 'sync-message', event, process_id, routing_id);
  std::vector<v8::Handle<v8::Value>> arguments;
  arguments.reserve(3 + args.GetSize());
  arguments.push_back(ToV8Value(channel));
  const base::Value* value;
  if (args.Get(0, &value))
    arguments.push_back(converter->ToV8Value(value, global_env->context()));
  arguments.push_back(event);
  arguments.push_back(v8::Integer::New(process_id));
  arguments.push_back(v8::Integer::New(routing_id));

  for (size_t i = 1; i < args.GetSize(); i++) {
    const base::Value* value;
    if (args.Get(i, &value))
      arguments.push_back(converter->ToV8Value(value, global_env->context()));
  }

  node::MakeCallback(global_env->process_object(),
                     "emit",
                     arguments.size(),
                     &arguments[0]);
}

}  // namespace atom
