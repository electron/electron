// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/atom_api_context_bridge.h"

#include <set>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "shell/common/api/remote/object_life_monitor.h"
#include "shell/common/native_mate_converters/blink_converter.h"
#include "shell/common/native_mate_converters/callback_converter_deprecated.h"
#include "shell/common/native_mate_converters/once_callback.h"
#include "shell/common/promise_util.h"

namespace electron {

namespace api {

namespace {

content::RenderFrame* GetRenderFrame(const v8::Local<v8::Object>& value) {
  v8::Local<v8::Context> context = value->CreationContext();
  if (context.IsEmpty())
    return nullptr;
  blink::WebLocalFrame* frame = blink::WebLocalFrame::FrameForContext(context);
  if (!frame)
    return nullptr;
  return content::RenderFrame::FromWebFrame(frame);
}

std::map<content::RenderFrame*, context_bridge::RenderFramePersistenceStore*>
    store_map_;

context_bridge::RenderFramePersistenceStore* GetOrCreateStore(
    content::RenderFrame* render_frame) {
  auto it = store_map_.find(render_frame);
  if (it == store_map_.end()) {
    auto* store = new context_bridge::RenderFramePersistenceStore(render_frame);
    store_map_[render_frame] = store;
    return store;
  }
  return it->second;
}

// Sourced from "extensions/renderer/v8_schema_registry.cc"
// Recursively freezes every v8 object on |object|.
void DeepFreeze(const v8::Local<v8::Object>& object,
                const v8::Local<v8::Context>& context,
                std::set<int> frozen = std::set<int>()) {
  int hash = object->GetIdentityHash();
  if (frozen.find(hash) != frozen.end())
    return;
  frozen.insert(hash);

  v8::Local<v8::Array> property_names =
      object->GetOwnPropertyNames(context).ToLocalChecked();
  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> child =
        object->Get(context, property_names->Get(context, i).ToLocalChecked())
            .ToLocalChecked();
    if (child->IsObject())
      DeepFreeze(v8::Local<v8::Object>::Cast(child), context, frozen);
  }
  object->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen);
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

template <typename Sig>
v8::Local<v8::Value> BindRepeatingFunctionToV8(
    v8::Isolate* isolate,
    const base::RepeatingCallback<Sig>& val) {
  auto translater =
      base::BindRepeating(&mate::internal::NativeFunctionInvoker<Sig>::Go, val);
  return mate::internal::CreateFunctionFromTranslater(isolate, translater,
                                                      false);
}

v8::Local<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source,
    v8::Local<v8::Context> destination,
    v8::Local<v8::Value> value,
    context_bridge::RenderFramePersistenceStore* store) {
  // Check Cache
  auto cached_value = store->GetCachedProxiedObject(value);
  if (!cached_value.IsEmpty()) {
    return cached_value.ToLocalChecked();
  }

  // Proxy functions and monitor the lifetime in the new context to release
  // the global handle at the right time.
  if (value->IsFunction()) {
    auto func = v8::Local<v8::Function>::Cast(value);
    v8::Global<v8::Function> global_func(source->GetIsolate(), func);
    v8::Global<v8::Context> global_source(source->GetIsolate(), source);

    size_t func_id = store->take_func_id();
    store->functions()[func_id] =
        std::make_tuple(std::move(global_func), std::move(global_source));
    v8::Context::Scope destination_scope(destination);
    v8::Local<v8::Value> proxy_func = BindRepeatingFunctionToV8(
        destination->GetIsolate(),
        base::BindRepeating(&ProxyFunctionWrapper, store, func_id));
    FunctionLifeMonitor::BindTo(destination->GetIsolate(),
                                v8::Local<v8::Object>::Cast(proxy_func), store,
                                func_id);
    store->CacheProxiedObject(value, proxy_func);
    return proxy_func;
  }

  // Proxy promises as they have a safe and guaranteed memory lifecycle
  if (value->IsPromise()) {
    v8::Context::Scope destination_scope(destination);

    auto v8_promise = v8::Local<v8::Promise>::Cast(value);
    auto* promise =
        new util::Promise<v8::Local<v8::Value>>(destination->GetIsolate());
    v8::Local<v8::Promise> handle = promise->GetHandle();

    auto then_cb = base::BindOnce(
        [](util::Promise<v8::Local<v8::Value>>* promise, v8::Isolate* isolate,
           v8::Global<v8::Context> source, v8::Global<v8::Context> destination,
           context_bridge::RenderFramePersistenceStore* store,
           v8::Local<v8::Value> result) {
          promise->Resolve(PassValueToOtherContext(
              source.Get(isolate), destination.Get(isolate), result, store));
          delete promise;
        },
        promise, destination->GetIsolate(),
        v8::Global<v8::Context>(source->GetIsolate(), source),
        v8::Global<v8::Context>(destination->GetIsolate(), destination), store);
    auto catch_cb = base::BindOnce(
        [](util::Promise<v8::Local<v8::Value>>* promise, v8::Isolate* isolate,
           v8::Global<v8::Context> source, v8::Global<v8::Context> destination,
           context_bridge::RenderFramePersistenceStore* store,
           v8::Local<v8::Value> result) {
          promise->Reject(PassValueToOtherContext(
              source.Get(isolate), destination.Get(isolate), result, store));
          delete promise;
        },
        promise, destination->GetIsolate(),
        v8::Global<v8::Context>(source->GetIsolate(), source),
        v8::Global<v8::Context>(destination->GetIsolate(), destination), store);

    ignore_result(v8_promise->Then(
        source,
        v8::Local<v8::Function>::Cast(
            mate::ConvertToV8(destination->GetIsolate(), then_cb)),
        v8::Local<v8::Function>::Cast(
            mate::ConvertToV8(destination->GetIsolate(), catch_cb))));

    store->CacheProxiedObject(value, handle);
    return handle;
  }

  // Errors aren't serializable currently, we need to pull the message out and
  // re-construct in the destination context
  if (value->IsNativeError()) {
    v8::Context::Scope destination_context_scope(destination);
    return v8::Exception::Error(
        v8::Exception::CreateMessage(destination->GetIsolate(), value)->Get());
  }

  // Manually go through the array and pass each value individually into a new
  // array so that functions deep insidee arrays get proxied or arrays of
  // promises are proxied correctly.
  if (IsPlainArray(value)) {
    v8::Context::Scope destination_context_scope(destination);
    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(value);
    size_t length = arr->Length();
    v8::Local<v8::Array> cloned_arr =
        v8::Array::New(destination->GetIsolate(), length);
    for (size_t i = 0; i < length; i++) {
      ignore_result(cloned_arr->Set(
          destination, static_cast<int>(i),
          PassValueToOtherContext(source, destination,
                                  arr->Get(source, i).ToLocalChecked(),
                                  store)));
    }
    store->CacheProxiedObject(value, cloned_arr);
    return cloned_arr;
  }

  // Proxy all objects
  if (IsPlainObject(value)) {
    auto object_value = v8::Local<v8::Object>::Cast(value);
    return CreateProxyForAPI(object_value, source, destination, store);
  }

  // Serializable objects
  blink::CloneableMessage ret;
  {
    v8::Context::Scope source_context_scope(source);
    // TODO(MarshallOfSound): What do we do if serialization fails? Throw an
    // error here?
    if (!mate::ConvertFromV8(source->GetIsolate(), value, &ret))
      return v8::Null(destination->GetIsolate());
  }

  v8::Context::Scope destination_context_scope(destination);
  return mate::ConvertToV8(destination->GetIsolate(), ret);
}

v8::Local<v8::Value> ProxyFunctionWrapper(
    context_bridge::RenderFramePersistenceStore* store,
    size_t func_id,
    mate::Arguments* args) {
  // Context the proxy function was called from
  v8::Local<v8::Context> calling_context = args->isolate()->GetCurrentContext();
  // Context the function was created in
  v8::Local<v8::Context> func_owning_context =
      std::get<1>(store->functions()[func_id]).Get(args->isolate());

  v8::Context::Scope func_owning_context_scope(func_owning_context);
  v8::Local<v8::Function> func =
      (std::get<0>(store->functions()[func_id])).Get(args->isolate());

  std::vector<v8::Local<v8::Value>> original_args;
  std::vector<v8::Local<v8::Value>> proxied_args;
  args->GetRemaining(&original_args);

  for (auto value : original_args) {
    proxied_args.push_back(PassValueToOtherContext(
        calling_context, func_owning_context, value, store));
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
          !mate::ConvertFromV8(args->isolate(), message->Get(),
                               &error_message)) {
        error_message =
            "An unknown exception occurred in the isolated context, an error "
            "occurred but a valid exception was not thrown.";
      }
    }
  }

  if (did_error) {
    v8::Context::Scope calling_context_scope(calling_context);
    args->ThrowError(error_message);
    return v8::Local<v8::Object>();
  }

  if (maybe_return_value.IsEmpty())
    return v8::Undefined(args->isolate());

  auto return_value = maybe_return_value.ToLocalChecked();

  return PassValueToOtherContext(func_owning_context, calling_context,
                                 return_value, store);
}

v8::Local<v8::Object> CreateProxyForAPI(
    v8::Local<v8::Object>& api_object,
    v8::Local<v8::Context>& source_context,
    v8::Local<v8::Context>& target_context,
    context_bridge::RenderFramePersistenceStore* store) {
  mate::Dictionary api(source_context->GetIsolate(), api_object);
  mate::Dictionary proxy =
      mate::Dictionary::CreateEmpty(target_context->GetIsolate());
  store->CacheProxiedObject(api.GetHandle(), proxy.GetHandle());
  auto maybe_keys = api.GetHandle()->GetOwnPropertyNames(source_context);
  if (maybe_keys.IsEmpty())
    return proxy.GetHandle();
  auto keys = maybe_keys.ToLocalChecked();

  v8::Context::Scope target_context_scope(target_context);
  uint32_t length = keys->Length();
  std::string key_str;
  int key_int;
  for (uint32_t i = 0; i < length; i++) {
    v8::Local<v8::Value> key = keys->Get(target_context, i).ToLocalChecked();
    // Try get the key as a string
    if (!mate::ConvertFromV8(api.isolate(), key, &key_str)) {
      // Try get the key as an int
      if (!mate::ConvertFromV8(api.isolate(), key, &key_int))
        continue;
      // Convert the int to a string as they are interoperable as object keys
      key_str = base::NumberToString(key_int);
    }
    v8::Local<v8::Value> value;
    if (!api.Get(key_str, &value))
      continue;

    proxy.Set(key_str, PassValueToOtherContext(source_context, target_context,
                                               value, store));
  }

  return proxy.GetHandle();
}

#ifdef DCHECK_IS_ON
mate::Dictionary DebugGC(mate::Dictionary empty) {
  auto* render_frame = GetRenderFrame(empty.GetHandle());
  auto* store = GetOrCreateStore(render_frame);
  mate::Dictionary ret = mate::Dictionary::CreateEmpty(empty.isolate());
  ret.Set("functionCount", store->functions().size());
  auto* main_world_map = store->main_world_proxy_map();
  auto* isolated_world_map = store->isolated_world_proxy_map();
  ret.Set("mainWorldObjectCount", main_world_map->size());
  ret.Set("isolatedWorldObjectCount", isolated_world_map->size());
  int live_main = 0;
  for (auto iter = main_world_map->begin(); iter != main_world_map->end();
       iter++) {
    if (!iter->second.IsEmpty())
      live_main++;
  }
  int live_isolated = 0;
  for (auto iter = isolated_world_map->begin();
       iter != isolated_world_map->end(); iter++) {
    if (!iter->second.IsEmpty())
      live_isolated++;
  }
  ret.Set("mainWorlLiveObjectCount", live_main);
  ret.Set("isolatedWorldLiveObjectCount", live_isolated);
  return ret;
}
#endif

void ExposeAPIInMainWorld(const std::string& key,
                          v8::Local<v8::Object> api_object,
                          mate::Arguments* args) {
  auto* render_frame = GetRenderFrame(api_object);
  CHECK(render_frame);
  context_bridge::RenderFramePersistenceStore* store =
      GetOrCreateStore(render_frame);
  auto* frame = render_frame->GetWebFrame();
  CHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  mate::Dictionary global(main_context->GetIsolate(), main_context->Global());

  if (global.Has(key)) {
    args->ThrowError(
        "Cannot bind an API on top of an existing property on the window "
        "object");
    return;
  }

  v8::Local<v8::Context> isolated_context =
      frame->WorldScriptContext(args->isolate(), World::ISOLATED_WORLD);

  v8::Context::Scope main_context_scope(main_context);
  v8::Local<v8::Object> proxy =
      CreateProxyForAPI(api_object, isolated_context, main_context, store);
  DeepFreeze(proxy, main_context);
  global.SetReadOnlyNonConfigurable(key, proxy);
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("exposeAPIInMainWorld", &electron::api::ExposeAPIInMainWorld);
#ifdef DCHECK_IS_ON
  dict.SetMethod("_debugGCMaps", &electron::api::DebugGC);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_context_bridge, Initialize)
