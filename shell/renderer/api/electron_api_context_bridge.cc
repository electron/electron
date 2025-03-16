// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/electron_api_context_bridge.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/trace_event/trace_event.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "shell/common/world_ids.h"
#include "shell/renderer/preload_realm_context.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/blink/public/web/web_blob.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"  // nogncheck

namespace features {
BASE_FEATURE(kContextBridgeMutability,
             "ContextBridgeMutability",
             base::FEATURE_DISABLED_BY_DEFAULT);
}

namespace electron {

content::RenderFrame* GetRenderFrame(v8::Local<v8::Object> value);

namespace api {

namespace context_bridge {

const char kProxyFunctionPrivateKey[] = "electron_contextBridge_proxy_fn";
const char kProxyFunctionReceiverPrivateKey[] =
    "electron_contextBridge_proxy_fn_receiver";
const char kSupportsDynamicPropertiesPrivateKey[] =
    "electron_contextBridge_supportsDynamicProperties";
const char kOriginalFunctionPrivateKey[] = "electron_contextBridge_original_fn";

}  // namespace context_bridge

namespace {

static int kMaxRecursion = 1000;

// Returns true if |maybe| is both a value, and that value is true.
inline bool IsTrue(v8::Maybe<bool> maybe) {
  return maybe.IsJust() && maybe.FromJust();
}

// Sourced from "extensions/renderer/v8_schema_registry.cc"
// Recursively freezes every v8 object on |object|.
bool DeepFreeze(const v8::Local<v8::Object>& object,
                const v8::Local<v8::Context>& context,
                std::set<int> frozen = std::set<int>()) {
  int hash = object->GetIdentityHash();
  if (frozen.contains(hash))
    return true;
  frozen.insert(hash);

  v8::Local<v8::Array> property_names =
      object->GetOwnPropertyNames(context).ToLocalChecked();
  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> child =
        object->Get(context, property_names->Get(context, i).ToLocalChecked())
            .ToLocalChecked();
    if (child->IsObject() && !child->IsTypedArray()) {
      if (!DeepFreeze(child.As<v8::Object>(), context, frozen))
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
           object->IsSharedArrayBuffer() || object->IsGeneratorObject() ||
           object->IsWasmModuleObject() || object->IsWasmMemoryObject() ||
           object->IsModuleNamespaceObject() || object->IsProxy());
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

// Forward declare methods
void ProxyFunctionWrapper(const v8::FunctionCallbackInfo<v8::Value>& info);
v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const blink::ExecutionContext* source_execution_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth,
    BridgeErrorTarget error_target);

v8::MaybeLocal<v8::Value> PassValueToOtherContextInner(
    v8::Local<v8::Context> source_context,
    const blink::ExecutionContext* source_execution_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    v8::Local<v8::Value> parent_value,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth,
    BridgeErrorTarget error_target) {
  TRACE_EVENT0("electron", "ContextBridge::PassValueToOtherContextInner");
  if (recursion_depth >= kMaxRecursion) {
    v8::Context::Scope error_scope(error_target == BridgeErrorTarget::kSource
                                       ? source_context
                                       : destination_context);
    source_context->GetIsolate()->ThrowException(v8::Exception::TypeError(
        gin::StringToV8(source_context->GetIsolate(),
                        "Electron contextBridge recursion depth exceeded.  "
                        "Nested objects "
                        "deeper than 1000 are not supported.")));
    return {};
  }

  // Certain primitives always use the current contexts prototype and we can
  // pass these through directly which is significantly more performant than
  // copying them. This list of primitives is based on the classification of
  // "primitive value" as defined in the ECMA262 spec
  // https://tc39.es/ecma262/#sec-primitive-value
  if (value->IsString() || value->IsNumber() || value->IsNullOrUndefined() ||
      value->IsBoolean() || value->IsSymbol() || value->IsBigInt()) {
    return v8::MaybeLocal<v8::Value>(value);
  }

  // Check Cache
  auto cached_value = object_cache->GetCachedProxiedObject(value);
  if (!cached_value.IsEmpty()) {
    return cached_value;
  }

  // Proxy functions and monitor the lifetime in the new context to release
  // the global handle at the right time.
  if (value->IsFunction()) {
    auto func = value.As<v8::Function>();
    v8::MaybeLocal<v8::Value> maybe_original_fn = GetPrivate(
        source_context, func, context_bridge::kOriginalFunctionPrivateKey);

    {
      v8::Context::Scope destination_scope(destination_context);
      v8::Local<v8::Value> proxy_func;

      // If this function has already been sent over the bridge,
      // then it is being sent _back_ over the bridge and we can
      // simply return the original method here for performance reasons

      // For safety reasons we check if the destination context is the
      // creation context of the original method.  If it's not we proceed
      // with the proxy logic
      if (maybe_original_fn.ToLocal(&proxy_func) && proxy_func->IsFunction() &&
          proxy_func.As<v8::Object>()->GetCreationContextChecked() ==
              destination_context) {
        return v8::MaybeLocal<v8::Value>(proxy_func);
      }

      v8::Local<v8::Object> state =
          v8::Object::New(destination_context->GetIsolate());
      SetPrivate(destination_context, state,
                 context_bridge::kProxyFunctionPrivateKey, func);
      SetPrivate(destination_context, state,
                 context_bridge::kProxyFunctionReceiverPrivateKey,
                 parent_value);
      SetPrivate(destination_context, state,
                 context_bridge::kSupportsDynamicPropertiesPrivateKey,
                 gin::ConvertToV8(destination_context->GetIsolate(),
                                  support_dynamic_properties));

      if (!v8::Function::New(destination_context, ProxyFunctionWrapper, state)
               .ToLocal(&proxy_func))
        return {};
      SetPrivate(destination_context, proxy_func.As<v8::Object>(),
                 context_bridge::kOriginalFunctionPrivateKey, func);
      object_cache->CacheProxiedObject(value, proxy_func);
      return v8::MaybeLocal<v8::Value>(proxy_func);
    }
  }

  // Proxy promises as they have a safe and guaranteed memory lifecycle
  if (value->IsPromise()) {
    v8::Context::Scope destination_scope(destination_context);
    auto source_promise = value.As<v8::Promise>();
    // Make the promise a shared_ptr so that when the original promise is
    // freed the proxy promise is correctly freed as well instead of being
    // left dangling
    auto proxied_promise =
        std::make_shared<gin_helper::Promise<v8::Local<v8::Value>>>(
            destination_context->GetIsolate());
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
          v8::MaybeLocal<v8::Value> val;
          {
            v8::TryCatch try_catch(isolate);
            v8::Local<v8::Context> source_context =
                global_source_context.Get(isolate);
            val = PassValueToOtherContext(
                source_context, global_destination_context.Get(isolate), result,
                source_context->Global(), false,
                BridgeErrorTarget::kDestination);
            if (try_catch.HasCaught()) {
              if (try_catch.Message().IsEmpty()) {
                proxied_promise->RejectWithErrorMessage(
                    "An error was thrown while sending a promise result over "
                    "the context bridge but it was not actually an Error "
                    "object. This normally means that a promise was resolved "
                    "with a value that is not supported by the Context "
                    "Bridge.");
              } else {
                proxied_promise->Reject(
                    v8::Exception::Error(try_catch.Message()->Get()));
              }
              return;
            }
          }
          DCHECK(!val.IsEmpty());
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
          v8::MaybeLocal<v8::Value> val;
          {
            v8::TryCatch try_catch(isolate);
            v8::Local<v8::Context> source_context =
                global_source_context.Get(isolate);
            val = PassValueToOtherContext(
                source_context, global_destination_context.Get(isolate), result,
                source_context->Global(), false,
                BridgeErrorTarget::kDestination);
            if (try_catch.HasCaught()) {
              if (try_catch.Message().IsEmpty()) {
                proxied_promise->RejectWithErrorMessage(
                    "An error was thrown while sending a promise rejection "
                    "over the context bridge but it was not actually an Error "
                    "object. This normally means that a promise was rejected "
                    "with a value that is not supported by the Context "
                    "Bridge.");
              } else {
                proxied_promise->Reject(
                    v8::Exception::Error(try_catch.Message()->Get()));
              }
              return;
            }
          }
          if (!val.IsEmpty())
            proxied_promise->Reject(val.ToLocalChecked());
        },
        proxied_promise, destination_context->GetIsolate(),
        std::move(global_catch_source_context),
        std::move(global_catch_destination_context));

    std::ignore = source_promise->Then(
        source_context,
        gin::ConvertToV8(destination_context->GetIsolate(), std::move(then_cb))
            .As<v8::Function>(),
        gin::ConvertToV8(destination_context->GetIsolate(), std::move(catch_cb))
            .As<v8::Function>());

    object_cache->CacheProxiedObject(value, proxied_promise_handle);
    return v8::MaybeLocal<v8::Value>(proxied_promise_handle);
  }

  // Errors aren't serializable currently, we need to pull the message out and
  // re-construct in the destination context
  if (value->IsNativeError()) {
    v8::Context::Scope destination_context_scope(destination_context);
    // We should try to pull "message" straight off of the error as a
    // v8::Message includes some pretext that can get duplicated each time it
    // crosses the bridge we fallback to the v8::Message approach if we can't
    // pull "message" for some reason
    v8::MaybeLocal<v8::Value> maybe_message = value.As<v8::Object>()->Get(
        source_context,
        gin::ConvertToV8(source_context->GetIsolate(), "message"));
    v8::Local<v8::Value> message;
    if (maybe_message.ToLocal(&message) && message->IsString()) {
      return v8::MaybeLocal<v8::Value>(
          v8::Exception::Error(message.As<v8::String>()));
    }
    return v8::MaybeLocal<v8::Value>(v8::Exception::Error(
        v8::Exception::CreateMessage(destination_context->GetIsolate(), value)
            ->Get()));
  }

  // Manually go through the array and pass each value individually into a new
  // array so that functions deep inside arrays get proxied or arrays of
  // promises are proxied correctly.
  if (IsPlainArray(value)) {
    v8::Context::Scope destination_context_scope(destination_context);
    v8::Local<v8::Array> arr = value.As<v8::Array>();
    size_t length = arr->Length();
    v8::Local<v8::Array> cloned_arr =
        v8::Array::New(destination_context->GetIsolate(), length);
    for (size_t i = 0; i < length; i++) {
      auto value_for_array = PassValueToOtherContextInner(
          source_context, source_execution_context, destination_context,
          arr->Get(source_context, i).ToLocalChecked(), value, object_cache,
          support_dynamic_properties, recursion_depth + 1, error_target);
      if (value_for_array.IsEmpty())
        return {};

      if (!IsTrue(cloned_arr->Set(destination_context, static_cast<int>(i),
                                  value_for_array.ToLocalChecked()))) {
        return {};
      }
    }
    object_cache->CacheProxiedObject(value, cloned_arr);
    return v8::MaybeLocal<v8::Value>(cloned_arr);
  }

  // Clone certain DOM APIs only within Window contexts.
  if (source_execution_context->IsWindow()) {
    // Custom logic to "clone" Element references
    blink::WebElement elem = blink::WebElement::FromV8Value(
        destination_context->GetIsolate(), value);
    if (!elem.IsNull()) {
      v8::Context::Scope destination_context_scope(destination_context);
      return v8::MaybeLocal<v8::Value>(
          elem.ToV8Value(destination_context->GetIsolate()));
    }

    // Custom logic to "clone" Blob references
    blink::WebBlob blob =
        blink::WebBlob::FromV8Value(destination_context->GetIsolate(), value);
    if (!blob.IsNull()) {
      v8::Context::Scope destination_context_scope(destination_context);
      return v8::MaybeLocal<v8::Value>(
          blob.ToV8Value(destination_context->GetIsolate()));
    }
  }

  // Proxy all objects
  if (IsPlainObject(value)) {
    auto object_value = value.As<v8::Object>();
    auto passed_value = CreateProxyForAPI(
        object_value, source_context, source_execution_context,
        destination_context, object_cache, support_dynamic_properties,
        recursion_depth + 1, error_target);
    if (passed_value.IsEmpty())
      return {};
    return v8::MaybeLocal<v8::Value>(passed_value.ToLocalChecked());
  }

  // Serializable objects
  blink::CloneableMessage ret;
  {
    v8::Local<v8::Context> error_context =
        error_target == BridgeErrorTarget::kSource ? source_context
                                                   : destination_context;
    v8::Context::Scope error_scope(error_context);
    // V8 serializer will throw an error if required
    if (!gin::ConvertFromV8(error_context->GetIsolate(), value, &ret)) {
      return {};
    }
  }

  {
    v8::Context::Scope destination_context_scope(destination_context);
    v8::Local<v8::Value> cloned_value =
        gin::ConvertToV8(destination_context->GetIsolate(), ret);
    object_cache->CacheProxiedObject(value, cloned_value);
    return v8::MaybeLocal<v8::Value>(cloned_value);
  }
}

v8::MaybeLocal<v8::Value> PassValueToOtherContext(
    v8::Local<v8::Context> source_context,
    v8::Local<v8::Context> destination_context,
    v8::Local<v8::Value> value,
    v8::Local<v8::Value> parent_value,
    bool support_dynamic_properties,
    BridgeErrorTarget error_target,
    context_bridge::ObjectCache* existing_object_cache) {
  TRACE_EVENT0("electron", "ContextBridge::PassValueToOtherContext");

  context_bridge::ObjectCache local_object_cache;
  context_bridge::ObjectCache* object_cache =
      existing_object_cache ? existing_object_cache : &local_object_cache;

  const blink::ExecutionContext* source_execution_context =
      blink::ExecutionContext::From(source_context);
  DCHECK(source_execution_context);
  return PassValueToOtherContextInner(
      source_context, source_execution_context, destination_context, value,
      parent_value, object_cache, support_dynamic_properties, 0, error_target);
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
  v8::MaybeLocal<v8::Value> maybe_recv = GetPrivate(
      calling_context, data, context_bridge::kProxyFunctionReceiverPrivateKey);
  v8::Local<v8::Value> func_value;
  if (sdp_value.IsEmpty() || maybe_func.IsEmpty() || maybe_recv.IsEmpty() ||
      !gin::ConvertFromV8(args.isolate(), sdp_value.ToLocalChecked(),
                          &support_dynamic_properties) ||
      !maybe_func.ToLocal(&func_value))
    return;

  v8::Local<v8::Function> func = func_value.As<v8::Function>();
  v8::Local<v8::Context> func_owning_context =
      func->GetCreationContextChecked();

  {
    v8::Context::Scope func_owning_context_scope(func_owning_context);

    // Cache duplicate arguments as the same proxied value.
    context_bridge::ObjectCache object_cache;

    std::vector<v8::Local<v8::Value>> original_args;
    std::vector<v8::Local<v8::Value>> proxied_args;
    args.GetRemaining(&original_args);

    for (auto value : original_args) {
      auto arg = PassValueToOtherContext(
          calling_context, func_owning_context, value,
          calling_context->Global(), support_dynamic_properties,
          BridgeErrorTarget::kSource, &object_cache);
      if (arg.IsEmpty())
        return;
      proxied_args.push_back(arg.ToLocalChecked());
    }

    v8::MaybeLocal<v8::Value> maybe_return_value;
    bool did_error = false;
    v8::Local<v8::Value> error_message;
    {
      v8::TryCatch try_catch(args.isolate());
      maybe_return_value =
          func->Call(func_owning_context, maybe_recv.ToLocalChecked(),
                     proxied_args.size(), proxied_args.data());
      if (try_catch.HasCaught()) {
        did_error = true;
        v8::Local<v8::Value> exception = try_catch.Exception();

        const char err_msg[] =
            "An unknown exception occurred in the isolated context, an error "
            "occurred but a valid exception was not thrown.";

        if (!exception->IsNull() && exception->IsObject()) {
          v8::MaybeLocal<v8::Value> maybe_message =
              exception.As<v8::Object>()->Get(
                  func_owning_context,
                  gin::ConvertToV8(args.isolate(), "message"));

          if (!maybe_message.ToLocal(&error_message) ||
              !error_message->IsString()) {
            error_message = gin::StringToV8(args.isolate(), err_msg);
          }
        } else {
          error_message = gin::StringToV8(args.isolate(), err_msg);
        }
      }
    }

    if (did_error) {
      v8::Context::Scope calling_context_scope(calling_context);
      args.isolate()->ThrowException(
          v8::Exception::Error(error_message.As<v8::String>()));
      return;
    }

    if (maybe_return_value.IsEmpty())
      return;

    // In the case where we encountered an exception converting the return value
    // of the function we need to ensure that the exception / thrown value is
    // safely transferred from the function_owning_context (where it was thrown)
    // into the calling_context (where it needs to be thrown) To do this we pull
    // the message off the exception and later re-throw it in the right context.
    // In some cases the caught thing is not an exception i.e. it's technically
    // valid to `throw 123`.  In these cases to avoid infinite
    // PassValueToOtherContext recursion we bail early as being unable to send
    // the value from one context to the other.
    // TODO(MarshallOfSound): In this case and other cases where the error can't
    // be sent _across_ worlds we should probably log it globally in some way to
    // allow easier debugging.  This is not trivial though so is left to a
    // future change.
    bool did_error_converting_result = false;
    v8::MaybeLocal<v8::Value> ret;
    v8::Local<v8::String> exception;
    {
      v8::TryCatch try_catch(args.isolate());
      ret = PassValueToOtherContext(
          func_owning_context, calling_context,
          maybe_return_value.ToLocalChecked(), func_owning_context->Global(),
          support_dynamic_properties, BridgeErrorTarget::kDestination);
      if (try_catch.HasCaught()) {
        did_error_converting_result = true;
        if (!try_catch.Message().IsEmpty()) {
          exception = try_catch.Message()->Get();
        }
      }
    }
    if (did_error_converting_result) {
      v8::Context::Scope calling_context_scope(calling_context);
      if (exception.IsEmpty()) {
        const char err_msg[] =
            "An unknown exception occurred while sending a function return "
            "value over the context bridge, an error "
            "occurred but a valid exception was not thrown.";
        args.isolate()->ThrowException(v8::Exception::Error(
            gin::StringToV8(args.isolate(), err_msg).As<v8::String>()));
      } else {
        args.isolate()->ThrowException(v8::Exception::Error(exception));
      }
      return;
    }
    DCHECK(!ret.IsEmpty());
    if (ret.IsEmpty())
      return;
    info.GetReturnValue().Set(ret.ToLocalChecked());
  }
}

v8::MaybeLocal<v8::Object> CreateProxyForAPI(
    const v8::Local<v8::Object>& api_object,
    const v8::Local<v8::Context>& source_context,
    const blink::ExecutionContext* source_execution_context,
    const v8::Local<v8::Context>& destination_context,
    context_bridge::ObjectCache* object_cache,
    bool support_dynamic_properties,
    int recursion_depth,
    BridgeErrorTarget error_target) {
  gin_helper::Dictionary api(source_context->GetIsolate(), api_object);

  {
    v8::Context::Scope destination_context_scope(destination_context);
    auto proxy =
        gin_helper::Dictionary::CreateEmpty(destination_context->GetIsolate());
    object_cache->CacheProxiedObject(api.GetHandle(), proxy.GetHandle());
    auto maybe_keys = api.GetHandle()->GetOwnPropertyNames(
        source_context, static_cast<v8::PropertyFilter>(v8::ONLY_ENUMERABLE));
    if (maybe_keys.IsEmpty())
      return v8::MaybeLocal<v8::Object>(proxy.GetHandle());
    auto keys = maybe_keys.ToLocalChecked();

    uint32_t length = keys->Length();
    for (uint32_t i = 0; i < length; i++) {
      v8::Local<v8::Value> key =
          keys->Get(destination_context, i).ToLocalChecked();

      if (support_dynamic_properties) {
        v8::Context::Scope source_context_scope(source_context);
        auto maybe_desc = api.GetHandle()->GetOwnPropertyDescriptor(
            source_context, key.As<v8::Name>());
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
            v8::Context::Scope inner_destination_context_scope(
                destination_context);
            v8::Local<v8::Value> getter_proxy;
            v8::Local<v8::Value> setter_proxy;
            if (!getter.IsEmpty()) {
              if (!PassValueToOtherContextInner(
                       source_context, source_execution_context,
                       destination_context, getter, api.GetHandle(),
                       object_cache, support_dynamic_properties, 1,
                       error_target)
                       .ToLocal(&getter_proxy))
                continue;
            }
            if (!setter.IsEmpty()) {
              if (!PassValueToOtherContextInner(
                       source_context, source_execution_context,
                       destination_context, setter, api.GetHandle(),
                       object_cache, support_dynamic_properties, 1,
                       error_target)
                       .ToLocal(&setter_proxy))
                continue;
            }

            v8::PropertyDescriptor prop_desc(getter_proxy, setter_proxy);
            std::ignore = proxy.GetHandle()->DefineProperty(
                destination_context, key.As<v8::Name>(), prop_desc);
          }
          continue;
        }
      }
      v8::Local<v8::Value> value;
      if (!api.Get(key, &value))
        continue;

      auto passed_value = PassValueToOtherContextInner(
          source_context, source_execution_context, destination_context, value,
          api.GetHandle(), object_cache, support_dynamic_properties,
          recursion_depth + 1, error_target);
      if (passed_value.IsEmpty())
        return {};
      proxy.Set(key, passed_value.ToLocalChecked());
    }

    return proxy.GetHandle();
  }
}

namespace {

void ExposeAPI(v8::Isolate* isolate,
               v8::Local<v8::Context> source_context,
               v8::Local<v8::Context> target_context,
               const std::string& key,
               v8::Local<v8::Value> api,
               gin_helper::Arguments* args) {
  DCHECK(!target_context.IsEmpty());
  v8::Context::Scope target_context_scope(target_context);
  gin_helper::Dictionary global(target_context->GetIsolate(),
                                target_context->Global());

  if (global.Has(key)) {
    args->ThrowError(
        "Cannot bind an API on top of an existing property on the window "
        "object");
    return;
  }

  v8::MaybeLocal<v8::Value> maybe_proxy = PassValueToOtherContext(
      source_context, target_context, api, source_context->Global(), false,
      BridgeErrorTarget::kSource);
  if (maybe_proxy.IsEmpty())
    return;
  auto proxy = maybe_proxy.ToLocalChecked();

  if (base::FeatureList::IsEnabled(features::kContextBridgeMutability)) {
    global.Set(key, proxy);
    return;
  }

  if (proxy->IsObject() && !proxy->IsTypedArray() &&
      !DeepFreeze(proxy.As<v8::Object>(), target_context))
    return;

  global.SetReadOnlyNonConfigurable(key, proxy);
}

// Attempt to get the target context based on the current context.
//
// For render frames, this is either the main world (0) or an arbitrary
// world ID. For service workers, Electron only supports one isolated
// context and the main worker context. Anything else is invalid.
v8::MaybeLocal<v8::Context> GetTargetContext(v8::Isolate* isolate,
                                             const int world_id) {
  v8::Local<v8::Context> source_context = isolate->GetCurrentContext();
  v8::MaybeLocal<v8::Context> maybe_target_context;

  blink::ExecutionContext* execution_context =
      blink::ExecutionContext::From(source_context);
  if (execution_context->IsWindow()) {
    auto* render_frame = GetRenderFrame(source_context->Global());
    CHECK(render_frame);
    auto* frame = render_frame->GetWebFrame();
    CHECK(frame);

    maybe_target_context =
        world_id == WorldIDs::MAIN_WORLD_ID
            ? frame->MainWorldScriptContext()
            : frame->GetScriptContextFromWorldId(isolate, world_id);
  } else if (execution_context->IsShadowRealmGlobalScope()) {
    if (world_id != WorldIDs::MAIN_WORLD_ID) {
      isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
          isolate, "Isolated worlds are not supported in preload realms.")));
      return maybe_target_context;
    }
    maybe_target_context =
        electron::preload_realm::GetInitiatorContext(source_context);
  } else {
    NOTREACHED();
  }

  CHECK(!maybe_target_context.IsEmpty());
  return maybe_target_context;
}

void ExposeAPIInWorld(v8::Isolate* isolate,
                      const int world_id,
                      const std::string& key,
                      v8::Local<v8::Value> api,
                      gin_helper::Arguments* args) {
  TRACE_EVENT2("electron", "ContextBridge::ExposeAPIInWorld", "key", key,
               "worldId", world_id);
  v8::Local<v8::Context> source_context = isolate->GetCurrentContext();
  CHECK(!source_context.IsEmpty());
  v8::MaybeLocal<v8::Context> maybe_target_context =
      GetTargetContext(isolate, world_id);
  if (maybe_target_context.IsEmpty())
    return;
  v8::Local<v8::Context> target_context = maybe_target_context.ToLocalChecked();
  ExposeAPI(isolate, source_context, target_context, key, api, args);
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
  if (key_path.empty())
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
    v8::Local<v8::Context> source_context = value->GetCreationContextChecked();
    v8::MaybeLocal<v8::Value> maybe_proxy = PassValueToOtherContext(
        source_context, main_context, value, source_context->Global(),
        support_dynamic_properties, BridgeErrorTarget::kSource);
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
  if (key_path.empty())
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
      v8::Local<v8::Context> source_context =
          getter->GetCreationContextChecked();
      v8::MaybeLocal<v8::Value> maybe_getter_proxy = PassValueToOtherContext(
          source_context, main_context, getter, source_context->Global(), false,
          BridgeErrorTarget::kSource);
      DCHECK(!maybe_getter_proxy.IsEmpty());
      getter_proxy = maybe_getter_proxy.ToLocalChecked();
    }
    if (!setter->IsNullOrUndefined() && setter->IsObject()) {
      v8::Local<v8::Context> source_context =
          getter->GetCreationContextChecked();
      v8::MaybeLocal<v8::Value> maybe_setter_proxy = PassValueToOtherContext(
          source_context, main_context, setter, source_context->Global(), false,
          BridgeErrorTarget::kSource);
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

// Serialize script to be executed in the given world.
v8::Local<v8::Value> ExecuteInWorld(v8::Isolate* isolate,
                                    const int world_id,
                                    gin_helper::Arguments* args) {
  // Get context of caller
  v8::Local<v8::Context> source_context = isolate->GetCurrentContext();

  // Get execution script argument
  gin_helper::Dictionary exec_script;
  if (args->Length() >= 1 && !args->GetNext(&exec_script)) {
    gin_helper::ErrorThrower(args->isolate()).ThrowError("Invalid script");
    return v8::Undefined(isolate);
  }

  // Get "func" from execution script
  v8::Local<v8::Function> func;
  if (!exec_script.Get("func", &func)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Function 'func' is required in script");
    return v8::Undefined(isolate);
  }

  // Get optional "args" from execution script
  v8::Local<v8::Array> args_array;
  v8::Local<v8::Value> args_value;
  if (exec_script.Get("args", &args_value)) {
    if (!args_value->IsArray()) {
      gin_helper::ErrorThrower(isolate).ThrowError("'args' must be an array");
      return v8::Undefined(isolate);
    }
    args_array = args_value.As<v8::Array>();
  }

  // Serialize the function
  std::string function_str;
  {
    v8::Local<v8::String> serialized_function;
    if (!func->FunctionProtoToString(isolate->GetCurrentContext())
             .ToLocal(&serialized_function)) {
      gin_helper::ErrorThrower(isolate).ThrowError(
          "Failed to serialize function");
      return v8::Undefined(isolate);
    }
    // If ToLocal() succeeds, this should always be a string.
    CHECK(gin::Converter<std::string>::FromV8(isolate, serialized_function,
                                              &function_str));
  }

  // Get the target context
  v8::MaybeLocal<v8::Context> maybe_target_context =
      GetTargetContext(isolate, world_id);
  v8::Local<v8::Context> target_context;
  if (!maybe_target_context.ToLocal(&target_context)) {
    isolate->ThrowException(v8::Exception::Error(gin::StringToV8(
        isolate,
        absl::StrFormat("Failed to get context for world %d", world_id))));
    return v8::Undefined(isolate);
  }

  // Compile the script
  v8::Local<v8::Script> compiled_script;
  {
    v8::Context::Scope target_scope(target_context);
    std::string error_message;
    v8::MaybeLocal<v8::Script> maybe_compiled_script;
    {
      v8::TryCatch try_catch(isolate);
      std::string return_func_code = absl::StrFormat("(%s)", function_str);
      maybe_compiled_script = v8::Script::Compile(
          target_context, gin::StringToV8(isolate, return_func_code));
      if (try_catch.HasCaught()) {
        // Must throw outside of TryCatch scope
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        error_message =
            *error ? *error : "Unknown error during script compilation";
      }
    }
    if (!maybe_compiled_script.ToLocal(&compiled_script)) {
      isolate->ThrowException(
          v8::Exception::Error(gin::StringToV8(isolate, error_message)));
      return v8::Undefined(isolate);
    }
  }

  // Run the script
  v8::Local<v8::Function> copied_func;
  {
    v8::Context::Scope target_scope(target_context);
    std::string error_message;
    v8::MaybeLocal<v8::Value> maybe_script_result;
    {
      v8::TryCatch try_catch(isolate);
      maybe_script_result = compiled_script->Run(target_context);
      if (try_catch.HasCaught()) {
        // Must throw outside of TryCatch scope
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        error_message =
            *error ? *error : "Unknown error during script execution";
      }
    }
    v8::Local<v8::Value> script_result;
    if (!maybe_script_result.ToLocal(&script_result)) {
      isolate->ThrowException(
          v8::Exception::Error(gin::StringToV8(isolate, error_message)));
      return v8::Undefined(isolate);
    }
    if (!script_result->IsFunction()) {
      isolate->ThrowException(v8::Exception::Error(
          gin::StringToV8(isolate,
                          "Expected script to result in a function but a "
                          "non-function type was found")));
      return v8::Undefined(isolate);
    }
    // Get copied function from the script result
    copied_func = script_result.As<v8::Function>();
  }

  // Proxy args to be passed into copied function
  std::vector<v8::Local<v8::Value>> proxied_args;
  {
    v8::Context::Scope target_scope(target_context);
    bool support_dynamic_properties = false;
    uint32_t args_length = args_array.IsEmpty() ? 0 : args_array->Length();

    // Cache duplicate arguments as the same proxied value.
    context_bridge::ObjectCache object_cache;

    for (uint32_t i = 0; i < args_length; ++i) {
      v8::Local<v8::Value> arg;
      if (!args_array->Get(source_context, i).ToLocal(&arg)) {
        gin_helper::ErrorThrower(isolate).ThrowError(
            absl::StrFormat("Failed to get argument at index %d", i));
        return v8::Undefined(isolate);
      }

      auto proxied_arg = PassValueToOtherContext(
          source_context, target_context, arg, source_context->Global(),
          support_dynamic_properties, BridgeErrorTarget::kSource,
          &object_cache);
      if (proxied_arg.IsEmpty()) {
        gin_helper::ErrorThrower(isolate).ThrowError(
            absl::StrFormat("Failed to proxy argument at index %d", i));
        return v8::Undefined(isolate);
      }
      proxied_args.push_back(proxied_arg.ToLocalChecked());
    }
  }

  // Call the function and get the result
  v8::Local<v8::Value> result;
  {
    v8::Context::Scope target_scope(target_context);
    std::string error_message;
    v8::MaybeLocal<v8::Value> maybe_result;
    {
      v8::TryCatch try_catch(isolate);
      maybe_result =
          copied_func->Call(isolate, target_context, v8::Null(isolate),
                            proxied_args.size(), proxied_args.data());
      if (try_catch.HasCaught()) {
        v8::String::Utf8Value error(isolate, try_catch.Exception());
        error_message =
            *error ? *error : "Unknown error during function execution";
      }
    }
    if (!maybe_result.ToLocal(&result)) {
      // Must throw outside of TryCatch scope
      isolate->ThrowException(
          v8::Exception::Error(gin::StringToV8(isolate, error_message)));
      return v8::Undefined(isolate);
    }
  }

  // Clone the result into the source/caller context
  v8::Local<v8::Value> cloned_result;
  {
    v8::Context::Scope source_scope(source_context);
    std::string error_message;
    v8::MaybeLocal<v8::Value> maybe_cloned_result;
    {
      v8::TryCatch try_catch(isolate);
      // Pass value from target context back to source context
      maybe_cloned_result = PassValueToOtherContext(
          target_context, source_context, result, target_context->Global(),
          false, BridgeErrorTarget::kSource);
      if (try_catch.HasCaught()) {
        v8::String::Utf8Value utf8(isolate, try_catch.Exception());
        error_message = *utf8 ? *utf8 : "Unknown error cloning result";
      }
    }
    if (!maybe_cloned_result.ToLocal(&cloned_result)) {
      // Must throw outside of TryCatch scope
      isolate->ThrowException(
          v8::Exception::Error(gin::StringToV8(isolate, error_message)));
      return v8::Undefined(isolate);
    }
  }
  return cloned_result;
}

}  // namespace

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("executeInWorld", &electron::api::ExecuteInWorld);
  dict.SetMethod("exposeAPIInWorld", &electron::api::ExposeAPIInWorld);
  dict.SetMethod("_overrideGlobalValueFromIsolatedWorld",
                 &electron::api::OverrideGlobalValueFromIsolatedWorld);
  dict.SetMethod("_overrideGlobalPropertyFromIsolatedWorld",
                 &electron::api::OverrideGlobalPropertyFromIsolatedWorld);
#if DCHECK_IS_ON()
  dict.Set("_isDebug", true);
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_renderer_context_bridge, Initialize)
