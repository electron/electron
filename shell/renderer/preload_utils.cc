// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/preload_utils.h"

#include "base/process/process.h"
#include "shell/common/gin_helper/arguments.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "v8/include/v8-context.h"

namespace electron::preload_utils {

namespace {

constexpr std::string_view kBindingCacheKey = "native-binding-cache";

v8::Local<v8::Object> GetBindingCache(v8::Isolate* isolate) {
  auto context = isolate->GetCurrentContext();
  gin_helper::Dictionary global(isolate, context->Global());
  v8::Local<v8::Value> cache;

  if (!global.GetHidden(kBindingCacheKey, &cache)) {
    cache = v8::Object::New(isolate);
    global.SetHidden(kBindingCacheKey, cache);
  }

  return cache->ToObject(context).ToLocalChecked();
}

}  // namespace

// adapted from node.cc
v8::Local<v8::Value> GetBinding(v8::Isolate* isolate,
                                v8::Local<v8::String> key,
                                gin_helper::Arguments* margs) {
  v8::Local<v8::Object> exports;
  std::string binding_key = gin::V8ToString(isolate, key);
  gin_helper::Dictionary cache(isolate, GetBindingCache(isolate));

  if (cache.Get(binding_key, &exports)) {
    return exports;
  }

  auto* mod = node::binding::get_linked_module(binding_key.c_str());

  if (!mod) {
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg), "No such binding: %s",
             binding_key.c_str());
    margs->ThrowError(errmsg);
    return exports;
  }

  exports = v8::Object::New(isolate);
  DCHECK_EQ(mod->nm_register_func, nullptr);
  DCHECK_NE(mod->nm_context_register_func, nullptr);
  mod->nm_context_register_func(exports, v8::Null(isolate),
                                isolate->GetCurrentContext(), mod->nm_priv);
  cache.Set(binding_key, exports);
  return exports;
}

v8::Local<v8::Value> CreatePreloadScript(v8::Isolate* isolate,
                                         v8::Local<v8::String> source) {
  auto context = isolate->GetCurrentContext();
  auto maybe_script = v8::Script::Compile(context, source);
  v8::Local<v8::Script> script;
  if (!maybe_script.ToLocal(&script))
    return {};
  return script->Run(context).ToLocalChecked();
}

double Uptime() {
  return (base::Time::Now() - base::Process::Current().CreationTime())
      .InSecondsF();
}

}  // namespace electron::preload_utils
