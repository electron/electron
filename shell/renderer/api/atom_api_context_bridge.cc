// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/atom_api_context_bridge.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/common/api/remote/object_life_monitor.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/renderer/api/context_bridge/render_frame_context_bridge_store.h"
#include "shell/renderer/atom_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace {

static int kMaxRecursion = 1000;

// Returns true if |maybe| is both a value, and that value is true.
inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

content::RenderFrame* GetRenderFrame(const v8::Local<v8::Object>& value) {
  v8::Local<v8::Context> context = value->CreationContext();
  if (context.IsEmpty())
    return nullptr;
  blink::WebLocalFrame* frame = blink::WebLocalFrame::FrameForContext(context);
  if (!frame)
    return nullptr;
  return content::RenderFrame::FromWebFrame(frame);
}

context_bridge::RenderFramePersistenceStore* GetOrCreateStore(
    content::RenderFrame* render_frame) {
  auto it = context_bridge::GetStoreMap().find(render_frame->GetRoutingID());
  if (it == context_bridge::GetStoreMap().end()) {
    auto* store = new context_bridge::RenderFramePersistenceStore(render_frame);
    context_bridge::GetStoreMap().emplace(render_frame->GetRoutingID(), store);
    return store;
  }
  return it->second;
}

// Sourced from "extensions/renderer/v8_schema_registry.cc"
// Recursively freezes every v8 object on |object|.
bool DeepFreeze(const v8::Local<v8::Object>& object,
                const v8::Local<v8::Context>& context,
                std::set<int> frozen = std::set<int>()) {
  int hash = object->GetIdentityHash();
  if (frozen.find(hash) != frozen.end())
    return true;
  frozen.insert(hash);

  v8::Local<v8::Array> property_names =
      object->GetOwnPropertyNames(context).ToLocalChecked();
  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> child =
        object->Get(context, property_names->Get(context, i).ToLocalChecked())
            .ToLocalChecked();
    if (child->IsObject() && !child->IsTypedArray()) {
      if (!DeepFreeze(v8::Local<v8::Object>::Cast(child), context, frozen))
        return false;
    }
  }
  return IsTrue(
      object->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen));
}

bool IsPlainObject(const v8::Local<v8::Value>& object) {
  if (!object->IsObject())
    return false;

  return !(object->IsNullOrUndefined() || object->IsDate() ||
           object->IsArgumentsObject() || object->IsBigIntObject() ||
           object->IsBooleanObject() || object->IsNumberObject() ||
           object->IsStringObject() || object->IsSymbolObject() ||
           object->IsNativeError() || object->IsRegExp() ||
           object->IsPromise() || object->IsMap() || object->IsSet() ||
           object->IsMapIterator() || object->IsSetIterator() ||
           object->IsWeakMap() || object->IsWeakSet() ||
           object->IsArrayBuffer() || object->IsArrayBufferView() ||
           object->IsArray() || object->IsDataView() ||
           object->IsSharedArrayBuffer() || object->IsProxy() ||
           object->IsWebAssemblyCompiledModule() ||
           object->IsModuleNamespaceObject());
}

bool IsPlainArray(const v8::Local<v8::Value>& arr) {
  if (!arr->IsArray())
    return false;

  return !arr->IsTypedArray();
}

class FunctionLifeMonitor final : public ObjectLifeMonitor {
 public:
  static void BindTo(v8::Isolate* isolate,
                     v8::Local<v8::Object> target,
                     context_bridge::RenderFramePersistenceStore* store,
                     size_t func_id) {
    new FunctionLifeMonitor(isolate, target, store, func_id);
  }

 protected:
  FunctionLifeMonitor(v8::Isolate* isolate,
                      v8::Local<v8::Object> target,
                      context_bridge::RenderFramePersistenceStore* store,
                      size_t func_id)
      : ObjectLifeMonitor(isolate, target), store_(store), func_id_(func_id) {}
  ~FunctionLifeMonitor() override = default;

  void RunDestructor() override { store_->functions().erase(func_id_); }

 private:
  context_bridge::RenderFramePersistenceStore* store_;
  size_t func_id_;
};

}  // namespace

v8::MaybeLocal<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    context_bridge::RenderFramePersistenceStore* store,
    int recursion_depth) {
  if (recursion_depth >= kMaxRecursion) {
    v8::Context::Scope source_scope(source_context);
    {
      source_context->GetIsolate()->ThrowException(v8::Exception::TypeError(
          gin::StringToV8(source_context->GetIsolate(),
                          "Electron contextBridge recursion depth exceeded.  "
                          "Nested objects "
                          "deeper than 1000 are not supported.")));
      return v8::MaybeLocal<v8::Value>();
    }
  }
  // Check Cache
  auto cached_value = store->GetCachedProxiedObject(value);
  if (!cached_value.IsEmpty()) {
    return cached_value;
  }

  // Proxy functions and monitor the lifetime in the new context to release
  // the global handle at the right time.
  if (value->IsFunction()) {
    auto func = v8::Local<v8::Function>::Cast(value);
    v8::Global<v8::Function> global_func(source_context->GetIsolate(), func);
    v8::Global<v8::Context> global_source(source_context->GetIsolate(),
                                          source_context);

    size_t func_id = store->take_func_id();
    store->functions()[func_id] =
        std::make_tuple(std::move(global_func), std::move(global_source));
    v8::Context::Scope destination_scope(destination_context);
    {
      v8::Local<v8::Value> proxy_func = gin_helper::CallbackToV8Leaked(
          destination_context->GetIsolate(),
          base::BindRepeating(&ProxyFunctionWrapper, store, func_id));
      FunctionLifeMonitor::BindTo(destination_context->GetIsolate(),
                                  v8::Local<v8::Object>::Cast(proxy_func),
                                  store, func_id);
      store->CacheProxiedObject(value, proxy_func);
      return v8::MaybeLocal<v8::Value>(proxy_func);
    }
  }

  // Proxy promises as they have a safe and guaranteed memory lifecycle
  if (value->IsPromise()) {
    v8::Context::Scope destination_scope(destination_context);
    {
      auto source_promise = v8::Local<v8::Promise>::Cast(value);
      auto* proxied_promise = new gin_helper::Promise<v8::Local<v8::Value>>(
          destination_context->GetIsolate());
      v8::Local<v8::Promise> proxied_promise_handle =
          proxied_promise->GetHandle();

      auto then_cb = base::BindOnce(
          [](gin_helper::Promise<v8::Local<v8::Value>>* proxied_promise,
             v8::Isolate* isolate,
             v8::Global<v8::Context> global_source_context,
             v8::Global<v8::Context> global_destination_context,
             context_bridge::RenderFramePersistenceStore* store,
             v8::Local<v8::Value> result) {
            auto val = PassValueToOtherContext(
                global_source_context.Get(isolate),
                global_destination_context.Get(isolate), result, store, 0);
            if (!val.IsEmpty())
              proxied_promise->Resolve(val.ToLocalChecked());
            delete proxied_promise;
          },
          proxied_promise, destination_context->GetIsolate(),
          v8::Global<v8::Context>(source_context->GetIsolate(), source_context),
          v8::Global<v8::Context>(destination_context->GetIsolate(),
                                  destination_context),
          store);
      auto catch_cb = base::BindOnce(
          [](gin_helper::Promise<v8::Local<v8::Value>>* proxied_promise,
             v8::Isolate* isolate,
             v8::Global<v8::Context> global_source_context,
             v8::Global<v8::Context> global_destination_context,
             context_bridge::RenderFramePersistenceStore* store,
             v8::Local<v8::Value> result) {
            auto val = PassValueToOtherContext(
                global_source_context.Get(isolate),
                global_destination_context.Get(isolate), result, store, 0);
            if (!val.IsEmpty())
              proxied_promise->Reject(val.ToLocalChecked());
            delete proxied_promise;
          },
          proxied_promise, destination_context->GetIsolate(),
          v8::Global<v8::Context>(source_context->GetIsolate(), source_context),
          v8::Global<v8::Context>(destination_context->GetIsolate(),
                                  destination_context),
          store);

      ignore_result(source_promise->Then(
          source_context,
          v8::Local<v8::Function>::Cast(
              gin::ConvertToV8(destination_context->GetIsolate(), then_cb)),
          v8::Local<v8::Function>::Cast(
              gin::ConvertToV8(destination_context->GetIsolate(), catch_cb))));

      store->CacheProxiedObject(value, proxied_promise_handle);
      return v8::MaybeLocal<v8::Value>(proxied_promise_handle);
    }
  }

  // Errors aren't serializable currently, we need to pull the message out and
  // re-construct in the destination context
  if (value->IsNativeError()) {
    v8::Context::Scope destination_context_scope(destination_context);
    return v8::MaybeLocal<v8::Value>(v8::Exception::Error(
        v8::Exception::CreateMessage(destination_context->GetIsolate(), value)
            ->Get()));
  }

  // Manually go through the array and pass each value individually into a new
  // array so that functions deep inside arrays get proxied or arrays of
  // promises are proxied correctly.
  if (IsPlainArray(value)) {
    v8::Context::Scope destination_context_scope(destination_context);
    {
      v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(value);
      size_t length = arr->Length();
      v8::Local<v8::Array> cloned_arr =
          v8::Array::New(destination_context->GetIsolate(), length);
      for (size_t i = 0; i < length; i++) {
        auto value_for_array = PassValueToOtherContext(
            source_context, destination_context,
            arr->Get(source_context, i).ToLocalChecked(), store,
            recursion_depth + 1);
        if (value_for_array.IsEmpty())
          return v8::MaybeLocal<v8::Value>();

        if (!IsTrue(cloned_arr->Set(destination_context, static_cast<int>(i),
                                    value_for_array.ToLocalChecked()))) {
          return v8::MaybeLocal<v8::Value>();
        }
      }
      store->CacheProxiedObject(value, cloned_arr);
      return v8::MaybeLocal<v8::Value>(cloned_arr);
    }
  }

  // Proxy all objects
  if (IsPlainObject(value)) {
    auto object_value = v8::Local<v8::Object>::Cast(value);
    auto passed_value =
        CreateProxyForAPI(object_value, source_context, destination_context,
                          store, recursion_depth + 1);
    if (passed_value.IsEmpty())
      return v8::MaybeLocal<v8::Value>();
    return v8::MaybeLocal<v8::Value>(passed_value.ToLocalChecked());
  }

  // Serializable objects
  blink::CloneableMessage ret;
  {
    v8::Context::Scope source_context_scope(source_context);
    {
      // V8 serializer will throw an error if required
      if (!gin::ConvertFromV8(source_context->GetIsolate(), value, &ret))
        return v8::MaybeLocal<v8::Value>();
    }
  }

  v8::Context::Scope destination_context_scope(destination_context);
  {
    v8::Local<v8::Value> cloned_value =
        gin::ConvertToV8(destination_context->GetIsolate(), ret);
    store->CacheProxiedObject(value, cloned_value);
    return v8::MaybeLocal<v8::Value>(cloned_value);
  }
}

v8::Local<v8::Value> ProxyFunctionWrapper(
    context_bridge::RenderFramePersistenceStore* store,
    size_t func_id,
    gin_helper::Arguments* args) {
  // Context the proxy function was called from
  v8::Local<v8::Context> calling_context = args->isolate()->GetCurrentContext();
  // Context the function was created in
  v8::Local<v8::Context> func_owning_context =
      std::get<1>(store->functions()[func_id]).Get(args->isolate());

  v8::Context::Scope func_owning_context_scope(func_owning_context);
  {
    v8::Local<v8::Function> func =
        (std::get<0>(store->functions()[func_id])).Get(args->isolate());

    std::vector<v8::Local<v8::Value>> original_args;
    std::vector<v8::Local<v8::Value>> proxied_args;
    args->GetRemaining(&original_args);

    for (auto value : original_args) {
      auto arg = PassValueToOtherContext(calling_context, func_owning_context,
                                         value, store, 0);
      if (arg.IsEmpty())
        return v8::Undefined(args->isolate());
      proxied_args.push_back(arg.ToLocalChecked());
    }

    v8::MaybeLocal<v8::Value> maybe_return_value;
    bool did_error = false;
    std::string error_message;
    {
      v8::TryCatch try_catch(args->isolate());
      maybe_return_value = func->Call(func_owning_context, func,
                                      proxied_args.size(), proxied_args.data());
      if (try_catch.HasCaught()) {
        did_error = true;
        auto message = try_catch.Message();

        if (message.IsEmpty() ||
            !gin::ConvertFromV8(args->isolate(), message->Get(),
                                &error_message)) {
          error_message =
              "An unknown exception occurred in the isolated context, an error "
              "occurred but a valid exception was not thrown.";
        }
      }
    }

    if (did_error) {
      v8::Context::Scope calling_context_scope(calling_context);
      {
        args->ThrowError(error_message);
        return v8::Local<v8::Object>();
      }
    }

    if (maybe_return_value.IsEmpty())
      return v8::Undefined(args->isolate());

    auto ret =
        PassValueToOtherContext(func_owning_context, calling_context,
                                maybe_return_value.ToLocalChecked(), store, 0);
    if (ret.IsEmpty())
      return v8::Undefined(args->isolate());
    return ret.ToLocalChecked();
  }
}

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::RenderFramePersistenceStore* store,
    int recursion_depth) {
  gin_helper::Dictionary api(source_context->GetIsolate(), api_object);
  gin_helper::Dictionary proxy =
      gin::Dictionary::CreateEmpty(destination_context->GetIsolate());
  store->CacheProxiedObject(api.GetHandle(), proxy.GetHandle());
  auto maybe_keys = api.GetHandle()->GetOwnPropertyNames(
      source_context,
      static_cast<v8::PropertyFilter>(v8::ONLY_ENUMERABLE | v8::SKIP_SYMBOLS),
      v8::KeyConversionMode::kConvertToString);
  if (maybe_keys.IsEmpty())
    return v8::MaybeLocal<v8::Object>(proxy.GetHandle());
  auto keys = maybe_keys.ToLocalChecked();

  v8::Context::Scope destination_context_scope(destination_context);
  {
    uint32_t length = keys->Length();
    std::string key_str;
    for (uint32_t i = 0; i < length; i++) {
      v8::Local<v8::Value> key =
          keys->Get(destination_context, i).ToLocalChecked();
      // Try get the key as a string
      if (!gin::ConvertFromV8(api.isolate(), key, &key_str)) {
        continue;
      }
      v8::Local<v8::Value> value;
      if (!api.Get(key_str, &value))
        continue;

      auto passed_value =
          PassValueToOtherContext(source_context, destination_context, value,
                                  store, recursion_depth + 1);
      if (passed_value.IsEmpty())
        return v8::MaybeLocal<v8::Object>();
      proxy.Set(key_str, passed_value.ToLocalChecked());
    }

    return proxy.GetHandle();
  }
}

#ifdef DCHECK_IS_ON
gin_helper::Dictionary DebugGC(gin_helper::Dictionary empty) {
  auto* render_frame = GetRenderFrame(empty.GetHandle());
  auto* store = GetOrCreateStore(render_frame);
  gin_helper::Dictionary ret = gin::Dictionary::CreateEmpty(empty.isolate());
  ret.Set("functionCount", store->functions().size());
  auto* proxy_map = store->proxy_map();
  ret.Set("objectCount", proxy_map->size() * 2);
  int live_from = 0;
  int live_proxy = 0;
  for (auto iter = proxy_map->begin(); iter != proxy_map->end(); iter++) {
    auto* node = iter->second;
    while (node) {
      if (!std::get<0>(node->pair).IsEmpty())
        live_from++;
      if (!std::get<1>(node->pair).IsEmpty())
        live_proxy++;
      node = node->next;
    }
  }
  ret.Set("liveFromValues", live_from);
  ret.Set("liveProxyValues", live_proxy);
  return ret;
}
#endif

void ExposeAPIInMainWorld(const std::string& key,
                          v8::Local<v8::Object> api_object,
                          gin_helper::Arguments* args) {
  auto* render_frame = GetRenderFrame(api_object);
  CHECK(render_frame);
  context_bridge::RenderFramePersistenceStore* store =
      GetOrCreateStore(render_frame);
  auto* frame = render_frame->GetWebFrame();
  CHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  gin_helper::Dictionary global(main_context->GetIsolate(),
                                main_context->Global());

  if (global.Has(key)) {
    args->ThrowError(
        "Cannot bind an API on top of an existing property on the window "
        "object");
    return;
  }

  v8::Local<v8::Context> isolated_context =
      frame->WorldScriptContext(args->isolate(), World::ISOLATED_WORLD);

  v8::Context::Scope main_context_scope(main_context);
  {
    v8::MaybeLocal<v8::Object> maybe_proxy =
        CreateProxyForAPI(api_object, isolated_context, main_context, store, 0);
    if (maybe_proxy.IsEmpty())
      return;
    auto proxy = maybe_proxy.ToLocalChecked();
    if (!DeepFreeze(proxy, main_context))
      return;

    global.SetReadOnlyNonConfigurable(key, proxy);
  }
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("exposeAPIInMainWorld", &electron::api::ExposeAPIInMainWorld);
#ifdef DCHECK_IS_ON
  dict.SetMethod("_debugGCMaps", &electron::api::DebugGC);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_context_bridge, Initialize)
