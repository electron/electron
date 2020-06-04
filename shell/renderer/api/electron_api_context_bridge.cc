// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/electron_api_context_bridge.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/common/api/object_life_monitor.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/world_ids.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace context_bridge {

const char* const kProxyFunctionPrivateKey = "electron_contextBridge_proxy_fn";
const char* const kSupportsDynamicPropertiesPrivateKey =
    "electron_contextBridge_supportsDynamicProperties";

}  // namespace context_bridge

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
           object->IsWasmModuleObject() || object->IsModuleNamespaceObject());
}

bool IsPlainArray(const v8::Local<v8::Value>& arr) {
  if (!arr->IsArray())
    return false;

  return !arr->IsTypedArray();
}

void SetPrivate(v8::Local<v8::Context> context,
                v8::Local<v8::Object> target,
                const std::string& key,
                v8::Local<v8::Value> value) {
  target
      ->SetPrivate(
          context,
          v8::Private::ForApi(context->GetIsolate(),
                              gin::StringToV8(context->GetIsolate(), key)),
          value)
      .Check();
}

v8::MaybeLocal<v8::Value> GetPrivate(v8::Local<v8::Context> context,
                                     v8::Local<v8::Object> target,
                                     const std::string& key) {
  return target->GetPrivate(
      context,
      v8::Private::ForApi(context->GetIsolate(),
                          gin::StringToV8(context->GetIsolate(), key)));
}

}  // namespace

v8::MaybeLocal<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth) {
  TRACE_EVENT0("electron", "ContextBridge::PassValueToOtherContext");
  if (recursion_depth >= kMaxRecursion) {
    v8::Context::Scope source_scope(source_context);
    source_context->GetIsolate()->ThrowException(v8::Exception::TypeError(
        gin::StringToV8(source_context->GetIsolate(),
                        "Electron contextBridge recursion depth exceeded.  "
                        "Nested objects "
                        "deeper than 1000 are not supported.")));
    return v8::MaybeLocal<v8::Value>();
  }
  // Check Cache
  auto cached_value = object_cache->GetCachedProxiedObject(value);
  if (!cached_value.IsEmpty()) {
    return cached_value;
  }

  // Proxy functions and monitor the lifetime in the new context to release
  // the global handle at the right time.
  if (value->IsFunction()) {
    auto func = v8::Local<v8::Function>::Cast(value);

    {
      v8::Context::Scope destination_scope(destination_context);
      v8::Local<v8::Object> state =
          v8::Object::New(destination_context->GetIsolate());
      SetPrivate(destination_context, state,
                 context_bridge::kProxyFunctionPrivateKey, func);
      SetPrivate(destination_context, state,
                 context_bridge::kSupportsDynamicPropertiesPrivateKey,
                 gin::ConvertToV8(destination_context->GetIsolate(),
                                  support_dynamic_properties));
      v8::Local<v8::Value> proxy_func;
      if (!v8::Function::New(destination_context, ProxyFunctionWrapper, state)
               .ToLocal(&proxy_func))
        return v8::MaybeLocal<v8::Value>();
      object_cache->CacheProxiedObject(value, proxy_func);
      return v8::MaybeLocal<v8::Value>(proxy_func);
    }
  }

  // Proxy promises as they have a safe and guaranteed memory lifecycle
  if (value->IsPromise()) {
    v8::Context::Scope destination_scope(destination_context);
    auto source_promise = v8::Local<v8::Promise>::Cast(value);
    // Make the promise a shared_ptr so that when the original promise is
    // freed the proxy promise is correctly freed as well instead of being
    // left dangling
    std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>> proxied_promise(
        new gin_helper::Promise<v8::Local<v8::Value>>(
            destination_context->GetIsolate()));
    v8::Local<v8::Promise> proxied_promise_handle =
        proxied_promise->GetHandle();

    v8::Global<v8::Context> global_then_source_context(
        source_context->GetIsolate(), source_context);
    v8::Global<v8::Context> global_then_destination_context(
        destination_context->GetIsolate(), destination_context);
    global_then_source_context.SetWeak();
    global_then_destination_context.SetWeak();
    auto then_cb = base::BindOnce(
        [](std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>>
               proxied_promise,
           v8::Isolate* isolate, v8::Global<v8::Context> global_source_context,
           v8::Global<v8::Context> global_destination_context,
           v8::Local<v8::Value> result) {
          if (global_source_context.IsEmpty() ||
              global_destination_context.IsEmpty())
            return;
          context_bridge::ObjectCache object_cache;
          auto val =
              PassValueToOtherContext(global_source_context.Get(isolate),
                                      global_destination_context.Get(isolate),
                                      result, &object_cache, false, 0);
          if (!val.IsEmpty())
            proxied_promise->Resolve(val.ToLocalChecked());
        },
        proxied_promise, destination_context->GetIsolate(),
        std::move(global_then_source_context),
        std::move(global_then_destination_context));

    v8::Global<v8::Context> global_catch_source_context(
        source_context->GetIsolate(), source_context);
    v8::Global<v8::Context> global_catch_destination_context(
        destination_context->GetIsolate(), destination_context);
    global_catch_source_context.SetWeak();
    global_catch_destination_context.SetWeak();
    auto catch_cb = base::BindOnce(
        [](std::shared_ptr<gin_helper::Promise<v8::Local<v8::Value>>>
               proxied_promise,
           v8::Isolate* isolate, v8::Global<v8::Context> global_source_context,
           v8::Global<v8::Context> global_destination_context,
           v8::Local<v8::Value> result) {
          if (global_source_context.IsEmpty() ||
              global_destination_context.IsEmpty())
            return;
          context_bridge::ObjectCache object_cache;
          auto val =
              PassValueToOtherContext(global_source_context.Get(isolate),
                                      global_destination_context.Get(isolate),
                                      result, &object_cache, false, 0);
          if (!val.IsEmpty())
            proxied_promise->Reject(val.ToLocalChecked());
        },
        proxied_promise, destination_context->GetIsolate(),
        std::move(global_catch_source_context),
        std::move(global_catch_destination_context));

    ignore_result(source_promise->Then(
        source_context,
        v8::Local<v8::Function>::Cast(
            gin::ConvertToV8(destination_context->GetIsolate(), then_cb)),
        v8::Local<v8::Function>::Cast(
            gin::ConvertToV8(destination_context->GetIsolate(), catch_cb))));

    object_cache->CacheProxiedObject(value, proxied_promise_handle);
    return v8::MaybeLocal<v8::Value>(proxied_promise_handle);
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
    v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(value);
    size_t length = arr->Length();
    v8::Local<v8::Array> cloned_arr =
        v8::Array::New(destination_context->GetIsolate(), length);
    for (size_t i = 0; i < length; i++) {
      auto value_for_array = PassValueToOtherContext(
          source_context, destination_context,
          arr->Get(source_context, i).ToLocalChecked(), object_cache,
          support_dynamic_properties, recursion_depth + 1);
      if (value_for_array.IsEmpty())
        return v8::MaybeLocal<v8::Value>();

      if (!IsTrue(cloned_arr->Set(destination_context, static_cast<int>(i),
                                  value_for_array.ToLocalChecked()))) {
        return v8::MaybeLocal<v8::Value>();
      }
    }
    object_cache->CacheProxiedObject(value, cloned_arr);
    return v8::MaybeLocal<v8::Value>(cloned_arr);
  }

  // Proxy all objects
  if (IsPlainObject(value)) {
    auto object_value = v8::Local<v8::Object>::Cast(value);
    auto passed_value = CreateProxyForAPI(
        object_value, source_context, destination_context, object_cache,
        support_dynamic_properties, recursion_depth + 1);
    if (passed_value.IsEmpty())
      return v8::MaybeLocal<v8::Value>();
    return v8::MaybeLocal<v8::Value>(passed_value.ToLocalChecked());
  }

  // Serializable objects
  blink::CloneableMessage ret;
  {
    v8::Context::Scope source_context_scope(source_context);
    // V8 serializer will throw an error if required
    if (!gin::ConvertFromV8(source_context->GetIsolate(), value, &ret))
      return v8::MaybeLocal<v8::Value>();
  }

  {
    v8::Context::Scope destination_context_scope(destination_context);
    v8::Local<v8::Value> cloned_value =
        gin::ConvertToV8(destination_context->GetIsolate(), ret);
    object_cache->CacheProxiedObject(value, cloned_value);
    return v8::MaybeLocal<v8::Value>(cloned_value);
  }
}

void ProxyFunctionWrapper(const v8::FunctionCallbackInfo<v8::Value>& info) {
  TRACE_EVENT0("electron", "ContextBridge::ProxyFunctionWrapper");
  CHECK(info.Data()->IsObject());
  v8::Local<v8::Object> data = info.Data().As<v8::Object>();
  bool support_dynamic_properties = false;
  gin::Arguments args(info);
  // Context the proxy function was called from
  v8::Local<v8::Context> calling_context = args.isolate()->GetCurrentContext();

  // Pull the original function and its context off of the data private key
  v8::MaybeLocal<v8::Value> sdp_value =
      GetPrivate(calling_context, data,
                 context_bridge::kSupportsDynamicPropertiesPrivateKey);
  v8::MaybeLocal<v8::Value> maybe_func = GetPrivate(
      calling_context, data, context_bridge::kProxyFunctionPrivateKey);
  v8::Local<v8::Value> func_value;
  if (sdp_value.IsEmpty() || maybe_func.IsEmpty() ||
      !gin::ConvertFromV8(args.isolate(), sdp_value.ToLocalChecked(),
                          &support_dynamic_properties) ||
      !maybe_func.ToLocal(&func_value))
    return;

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);
  v8::Local<v8::Context> func_owning_context = func->CreationContext();

  {
    v8::Context::Scope func_owning_context_scope(func_owning_context);
    context_bridge::ObjectCache object_cache;

    std::vector<v8::Local<v8::Value>> original_args;
    std::vector<v8::Local<v8::Value>> proxied_args;
    args.GetRemaining(&original_args);

    for (auto value : original_args) {
      auto arg =
          PassValueToOtherContext(calling_context, func_owning_context, value,
                                  &object_cache, support_dynamic_properties, 0);
      if (arg.IsEmpty())
        return;
      proxied_args.push_back(arg.ToLocalChecked());
    }

    v8::MaybeLocal<v8::Value> maybe_return_value;
    bool did_error = false;
    std::string error_message;
    {
      v8::TryCatch try_catch(args.isolate());
      maybe_return_value = func->Call(func_owning_context, func,
                                      proxied_args.size(), proxied_args.data());
      if (try_catch.HasCaught()) {
        did_error = true;
        auto message = try_catch.Message();

        if (message.IsEmpty() ||
            !gin::ConvertFromV8(args.isolate(), message->Get(),
                                &error_message)) {
          error_message =
              "An unknown exception occurred in the isolated context, an error "
              "occurred but a valid exception was not thrown.";
        }
      }
    }

    if (did_error) {
      v8::Context::Scope calling_context_scope(calling_context);
      args.isolate()->ThrowException(
          v8::Exception::Error(gin::StringToV8(args.isolate(), error_message)));
      return;
    }

    if (maybe_return_value.IsEmpty())
      return;

    auto ret =
        PassValueToOtherContext(func_owning_context, calling_context,
                                maybe_return_value.ToLocalChecked(),
                                &object_cache, support_dynamic_properties, 0);
    if (ret.IsEmpty())
      return;
    info.GetReturnValue().Set(ret.ToLocalChecked());
  }
}

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth) {
  gin_helper::Dictionary api(source_context->GetIsolate(), api_object);

  {
    v8::Context::Scope destination_context_scope(destination_context);
    gin_helper::Dictionary proxy =
        gin::Dictionary::CreateEmpty(destination_context->GetIsolate());
    object_cache->CacheProxiedObject(api.GetHandle(), proxy.GetHandle());
    auto maybe_keys = api.GetHandle()->GetOwnPropertyNames(
        source_context,
        static_cast<v8::PropertyFilter>(v8::ONLY_ENUMERABLE | v8::SKIP_SYMBOLS),
        v8::KeyConversionMode::kConvertToString);
    if (maybe_keys.IsEmpty())
      return v8::MaybeLocal<v8::Object>(proxy.GetHandle());
    auto keys = maybe_keys.ToLocalChecked();

    uint32_t length = keys->Length();
    std::string key_str;
    for (uint32_t i = 0; i < length; i++) {
      v8::Local<v8::Value> key =
          keys->Get(destination_context, i).ToLocalChecked();
      // Try get the key as a string
      if (!gin::ConvertFromV8(api.isolate(), key, &key_str)) {
        continue;
      }
      if (support_dynamic_properties) {
        v8::Context::Scope source_context_scope(source_context);
        auto maybe_desc = api.GetHandle()->GetOwnPropertyDescriptor(
            source_context, v8::Local<v8::Name>::Cast(key));
        v8::Local<v8::Value> desc_value;
        if (!maybe_desc.ToLocal(&desc_value) || !desc_value->IsObject())
          continue;
        gin_helper::Dictionary desc(api.isolate(), desc_value.As<v8::Object>());
        if (desc.Has("get") || desc.Has("set")) {
          v8::Local<v8::Value> getter;
          v8::Local<v8::Value> setter;
          desc.Get("get", &getter);
          desc.Get("set", &setter);

          {
            v8::Context::Scope destination_context_scope(destination_context);
            v8::Local<v8::Value> getter_proxy;
            v8::Local<v8::Value> setter_proxy;
            if (!getter.IsEmpty()) {
              if (!PassValueToOtherContext(source_context, destination_context,
                                           getter, object_cache, false, 1)
                       .ToLocal(&getter_proxy))
                continue;
            }
            if (!setter.IsEmpty()) {
              if (!PassValueToOtherContext(source_context, destination_context,
                                           setter, object_cache, false, 1)
                       .ToLocal(&setter_proxy))
                continue;
            }

            v8::PropertyDescriptor desc(getter_proxy, setter_proxy);
            ignore_result(proxy.GetHandle()->DefineProperty(
                destination_context, gin::StringToV8(api.isolate(), key_str),
                desc));
          }
          continue;
        }
      }
      v8::Local<v8::Value> value;
      if (!api.Get(key_str, &value))
        continue;

      auto passed_value = PassValueToOtherContext(
          source_context, destination_context, value, object_cache,
          support_dynamic_properties, recursion_depth + 1);
      if (passed_value.IsEmpty())
        return v8::MaybeLocal<v8::Object>();
      proxy.Set(key_str, passed_value.ToLocalChecked());
    }

    return proxy.GetHandle();
  }
}

void ExposeAPIInMainWorld(const std::string& key,
                          v8::Local<v8::Object> api_object,
                          gin_helper::Arguments* args) {
  TRACE_EVENT1("electron", "ContextBridge::ExposeAPIInMainWorld", "key", key);
  auto* render_frame = GetRenderFrame(api_object);
  CHECK(render_frame);
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
      frame->WorldScriptContext(args->isolate(), WorldIDs::ISOLATED_WORLD_ID);

  {
    context_bridge::ObjectCache object_cache;
    v8::Context::Scope main_context_scope(main_context);

    v8::MaybeLocal<v8::Object> maybe_proxy = CreateProxyForAPI(
        api_object, isolated_context, main_context, &object_cache, false, 0);
    if (maybe_proxy.IsEmpty())
      return;
    auto proxy = maybe_proxy.ToLocalChecked();
    if (!DeepFreeze(proxy, main_context))
      return;

    global.SetReadOnlyNonConfigurable(key, proxy);
  }
}

gin_helper::Dictionary TraceKeyPath(const gin_helper::Dictionary& start,
                                    const std::vector<std::string>& key_path) {
  gin_helper::Dictionary current = start;
  for (size_t i = 0; i < key_path.size() - 1; i++) {
    CHECK(current.Get(key_path[i], &current));
  }
  return current;
}

void OverrideGlobalValueFromIsolatedWorld(
    const std::vector<std::string>& key_path,
    v8::Local<v8::Object> value,
    bool support_dynamic_properties) {
  if (key_path.size() == 0)
    return;

  auto* render_frame = GetRenderFrame(value);
  CHECK(render_frame);
  auto* frame = render_frame->GetWebFrame();
  CHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  gin_helper::Dictionary global(main_context->GetIsolate(),
                                main_context->Global());

  const std::string final_key = key_path[key_path.size() - 1];
  gin_helper::Dictionary target_object = TraceKeyPath(global, key_path);

  {
    v8::Context::Scope main_context_scope(main_context);
    context_bridge::ObjectCache object_cache;
    v8::MaybeLocal<v8::Value> maybe_proxy =
        PassValueToOtherContext(value->CreationContext(), main_context, value,
                                &object_cache, support_dynamic_properties, 1);
    DCHECK(!maybe_proxy.IsEmpty());
    auto proxy = maybe_proxy.ToLocalChecked();

    target_object.Set(final_key, proxy);
  }
}

bool OverrideGlobalPropertyFromIsolatedWorld(
    const std::vector<std::string>& key_path,
    v8::Local<v8::Object> getter,
    v8::Local<v8::Value> setter,
    gin_helper::Arguments* args) {
  if (key_path.size() == 0)
    return false;

  auto* render_frame = GetRenderFrame(getter);
  CHECK(render_frame);
  auto* frame = render_frame->GetWebFrame();
  CHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  gin_helper::Dictionary global(main_context->GetIsolate(),
                                main_context->Global());

  const std::string final_key = key_path[key_path.size() - 1];
  v8::Local<v8::Object> target_object =
      TraceKeyPath(global, key_path).GetHandle();

  {
    v8::Context::Scope main_context_scope(main_context);
    context_bridge::ObjectCache object_cache;
    v8::Local<v8::Value> getter_proxy;
    v8::Local<v8::Value> setter_proxy;
    if (!getter->IsNullOrUndefined()) {
      v8::MaybeLocal<v8::Value> maybe_getter_proxy =
          PassValueToOtherContext(getter->CreationContext(), main_context,
                                  getter, &object_cache, false, 1);
      DCHECK(!maybe_getter_proxy.IsEmpty());
      getter_proxy = maybe_getter_proxy.ToLocalChecked();
    }
    if (!setter->IsNullOrUndefined() && setter->IsObject()) {
      v8::MaybeLocal<v8::Value> maybe_setter_proxy =
          PassValueToOtherContext(getter->CreationContext(), main_context,
                                  setter, &object_cache, false, 1);
      DCHECK(!maybe_setter_proxy.IsEmpty());
      setter_proxy = maybe_setter_proxy.ToLocalChecked();
    }

    v8::PropertyDescriptor desc(getter_proxy, setter_proxy);
    bool success = IsTrue(target_object->DefineProperty(
        main_context, gin::StringToV8(args->isolate(), final_key), desc));
    DCHECK(success);
    return success;
  }
}

bool IsCalledFromMainWorld(v8::Isolate* isolate) {
  auto* render_frame = GetRenderFrame(isolate->GetCurrentContext()->Global());
  CHECK(render_frame);
  auto* frame = render_frame->GetWebFrame();
  CHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  return isolate->GetCurrentContext() == main_context;
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
  dict.SetMethod("_overrideGlobalValueFromIsolatedWorld",
                 &electron::api::OverrideGlobalValueFromIsolatedWorld);
  dict.SetMethod("_overrideGlobalPropertyFromIsolatedWorld",
                 &electron::api::OverrideGlobalPropertyFromIsolatedWorld);
  dict.SetMethod("_isCalledFromMainWorld",
                 &electron::api::IsCalledFromMainWorld);
#ifdef DCHECK_IS_ON
  dict.Set("_isDebug", true);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_renderer_context_bridge, Initialize)
