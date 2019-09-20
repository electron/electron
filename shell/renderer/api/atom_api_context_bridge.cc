// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "extensions/renderer/v8_schema_registry.h"
#include "native_mate/dictionary.h"
#include "shell/common/native_mate_converters/callback_converter_deprecated.h"
#include "shell/common/native_mate_converters/once_callback.h"
#include "shell/common/native_mate_converters/value_converter.h"
#include "shell/common/node_includes.h"
#include "shell/common/promise_util.h"
#include "shell/renderer/atom_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace {

content::RenderFrame* GetRenderFrame(v8::Local<v8::Value> value) {
  v8::Local<v8::Context> context =
      v8::Local<v8::Object>::Cast(value)->CreationContext();
  if (context.IsEmpty())
    return nullptr;
  blink::WebLocalFrame* frame = blink::WebLocalFrame::FrameForContext(context);
  if (!frame)
    return nullptr;
  return content::RenderFrame::FromWebFrame(frame);
}

using FunctionContextPair =
    std::tuple<v8::Global<v8::Function>, v8::Global<v8::Context>>;

class RenderFramePersistenceStore final : public content::RenderFrameObserver {
 public:
  explicit RenderFramePersistenceStore(content::RenderFrame* render_frame)
      : content::RenderFrameObserver(render_frame) {}
  ~RenderFramePersistenceStore() final = default;

  bool HasReverseBindingStore() { return !reverse_binding_store_.IsEmpty(); }

  void SetReverseBindingStore(v8::Isolate* isolate,
                              v8::Local<v8::Object> store) {
    reverse_binding_store_.Reset(isolate, store);
  }

  mate::Dictionary GetReverseBindingsStore(v8::Isolate* isolate) {
    return mate::Dictionary(isolate, reverse_binding_store_.Get(isolate));
  }

  // RenderFrameObserver implementation.
  void OnDestruct() final {
    for (auto& tup : functions_) {
      std::get<0>(tup).Reset();
      std::get<1>(tup).Reset();
    }
    reverse_binding_store_.Reset();
    delete this;
  }

  std::vector<FunctionContextPair>& functions() { return functions_; }

 private:
  // { function, owning_context }[]
  std::vector<FunctionContextPair> functions_;
  v8::Global<v8::Object> reverse_binding_store_;
};

std::map<content::RenderFrame*, RenderFramePersistenceStore*> store_map_;

RenderFramePersistenceStore* GetOrCreateStore(
    content::RenderFrame* render_frame) {
  if (store_map_.find(render_frame) == store_map_.end()) {
    store_map_[render_frame] = new RenderFramePersistenceStore(render_frame);
  }
  return store_map_[render_frame];
}

// Sourced from "extensions/renderer/v8_schema_registry.cc"
// Recursively freezes every v8 object on |object|.
void DeepFreeze(const v8::Local<v8::Object>& object,
                const v8::Local<v8::Context>& context) {
  v8::Local<v8::Array> property_names =
      object->GetOwnPropertyNames(context).ToLocalChecked();
  for (uint32_t i = 0; i < property_names->Length(); ++i) {
    v8::Local<v8::Value> child =
        object->Get(context, property_names->Get(context, i).ToLocalChecked())
            .ToLocalChecked();
    if (child->IsObject())
      DeepFreeze(v8::Local<v8::Object>::Cast(child), context);
  }
  object->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen);
}

}  // namespace

v8::Local<v8::Value> PassValueToOtherContext(v8::Local<v8::Context> source,
                                             v8::Local<v8::Context> destination,
                                             v8::Local<v8::Value> value) {
  // Errors aren't serializable currently, we need to pull the message out and
  // re-construct in the destination context
  if (value->IsNativeError()) {
    v8::Context::Scope destination_context_scope(destination);
    return v8::Exception::Error(
        v8::Exception::CreateMessage(destination->GetIsolate(), value)->Get());
  }

  // Serializable objects
  // TODO(MarshallOfSound): Use the V8 serializer
  base::Value ret;
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

v8::Local<v8::Value> ProxyFunctionWrapper(RenderFramePersistenceStore* store,
                                          size_t func_index,
                                          mate::Arguments* args) {
  // Context the proxy function was called from
  v8::Local<v8::Context> calling_context = args->isolate()->GetCurrentContext();
  // Context the function was created in
  v8::Local<v8::Context> func_owning_context =
      std::get<1>(store->functions()[func_index]).Get(args->isolate());

  v8::Context::Scope func_owning_context_scope(func_owning_context);
  v8::Local<v8::Function> func =
      (std::get<0>(store->functions()[func_index])).Get(args->isolate());

  std::vector<v8::Local<v8::Value>> original_args;
  std::vector<v8::Local<v8::Value>> proxied_args;
  args->GetRemaining(&original_args);
  for (auto value : original_args) {
    proxied_args.push_back(
        PassValueToOtherContext(calling_context, func_owning_context, value));
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

  if (return_value->IsPromise()) {
    v8::Context::Scope calling_context_scope(calling_context);

    auto v8_promise = v8::Local<v8::Promise>::Cast(return_value);
    util::Promise<v8::Local<v8::Value>> promise(args->isolate());
    v8::Local<v8::Promise> handle = promise.GetHandle();

    v8::Local<v8::Value> promise_handler = mate::ConvertToV8(
        args->isolate(),
        base::BindOnce(
            [](electron::util::Promise<v8::Local<v8::Value>> p,
               v8::Isolate* isolate, v8::Global<v8::Context> source,
               v8::Global<v8::Context> destination,
               v8::Local<v8::Value> result) {
              if (p.GetHandle()->State() ==
                  v8::Promise::PromiseState::kFulfilled) {
                p.Resolve(PassValueToOtherContext(
                    source.Get(isolate), destination.Get(isolate), result));
              } else {
                p.Reject(PassValueToOtherContext(
                    source.Get(isolate), destination.Get(isolate), result));
              }
            },
            std::move(promise), args->isolate(),
            v8::Global<v8::Context>(args->isolate(), func_owning_context),
            v8::Global<v8::Context>(args->isolate(), calling_context)));
    v8::Local<v8::Function> promise_func =
        v8::Local<v8::Function>::Cast(promise_handler);

    ignore_result(
        v8_promise->Then(func_owning_context, promise_func, promise_func));

    return handle;
  }

  return PassValueToOtherContext(func_owning_context, calling_context,
                                 return_value);
}

mate::Dictionary CreateProxyForAPI(mate::Dictionary api,
                                   v8::Local<v8::Context> source_context,
                                   v8::Local<v8::Context> target_context,
                                   RenderFramePersistenceStore* store) {
  mate::Dictionary proxy =
      mate::Dictionary::CreateEmpty(target_context->GetIsolate());
  auto maybe_keys =
      api.GetHandle()->GetOwnPropertyNames(api.isolate()->GetCurrentContext());
  if (maybe_keys.IsEmpty())
    return proxy;
  auto keys = maybe_keys.ToLocalChecked();

  uint32_t length = keys->Length();
  std::string key_str;
  for (uint32_t i = 0; i < length; i++) {
    v8::Context::Scope source_context_scope(source_context);

    v8::Local<v8::Value> key = keys->Get(target_context, i).ToLocalChecked();
    if (!mate::ConvertFromV8(api.isolate(), key, &key_str))
      continue;
    v8::Local<v8::Value> value;
    if (!api.Get(key_str, &value))
      continue;
    if (value->IsFunction()) {
      auto func = v8::Local<v8::Function>::Cast(value);
      v8::Global<v8::Function> global_func(api.isolate(), func);
      v8::Global<v8::Context> global_source_context(
          source_context->GetIsolate(), source_context);

      store->functions().push_back(std::make_tuple(
          std::move(global_func), std::move(global_source_context)));
      size_t func_id = store->functions().size() - 1;
      {
        v8::Context::Scope target_context_scope(target_context);
        proxy.SetMethod(key_str, base::BindRepeating(&ProxyFunctionWrapper,
                                                     store, func_id));
      }
    } else if (value->IsObject() && !value->IsNullOrUndefined() &&
               !value->IsArray()) {
      mate::Dictionary sub_api(
          api.isolate(),
          value->ToObject(api.isolate()->GetCurrentContext()).ToLocalChecked());
      {
        v8::Context::Scope target_context_scope(target_context);
        proxy.Set(key_str, CreateProxyForAPI(sub_api, source_context,
                                             target_context, store));
      }
    } else {
      {
        v8::Context::Scope target_context_scope(target_context);
        proxy.Set(key_str, PassValueToOtherContext(source_context,
                                                   target_context, value));
      }
    }
  }

  return proxy;
}

void ExposeReverseBindingInIsolatedWorld(const std::string& key,
                                         mate::Dictionary reverse_api) {
  auto* render_frame = GetRenderFrame(reverse_api.GetHandle());
  RenderFramePersistenceStore* store = GetOrCreateStore(render_frame);
  DCHECK(store->HasReverseBindingStore());
  auto* frame = render_frame->GetWebFrame();
  DCHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  v8::Local<v8::Context> isolated_context =
      frame->WorldScriptContext(reverse_api.isolate(), World::ISOLATED_WORLD);

  {
    v8::Context::Scope isolated_context_scope(isolated_context);
    mate::Dictionary proxy =
        CreateProxyForAPI(reverse_api, main_context, isolated_context, store);
    DeepFreeze(proxy.GetHandle(), isolated_context);
    store->GetReverseBindingsStore(reverse_api.isolate())
        .SetReadOnlyNonConfigurable(key, proxy);
  }
}

void ExposeAPIInMainWorld(const std::string& key,
                          mate::Dictionary api,
                          base::Optional<mate::Dictionary> options) {
  auto* render_frame = GetRenderFrame(api.GetHandle());
  RenderFramePersistenceStore* store = GetOrCreateStore(render_frame);
  DCHECK(store->HasReverseBindingStore());
  auto* frame = render_frame->GetWebFrame();
  DCHECK(frame);
  v8::Local<v8::Context> main_context = frame->MainWorldScriptContext();
  mate::Dictionary global(main_context->GetIsolate(), main_context->Global());

  v8::Local<v8::Context> isolated_context =
      frame->WorldScriptContext(api.isolate(), World::ISOLATED_WORLD);

  bool allow_reverse_binding = false;
  if (options.has_value())
    options->Get("allowReverseBinding", &allow_reverse_binding);

  {
    v8::Context::Scope main_context_scope(main_context);
    mate::Dictionary proxy =
        CreateProxyForAPI(api, isolated_context, main_context, store);
    if (allow_reverse_binding) {
      proxy.SetMethod("bind", base::BindRepeating(
                                  &ExposeReverseBindingInIsolatedWorld, key));
    }
    DeepFreeze(proxy.GetHandle(), main_context);
    global.SetReadOnlyNonConfigurable(key, proxy);
  }
}

void SetReverseBindingStore(v8::Isolate* isolate,
                            v8::Local<v8::Value> reverse_bindings_store) {
  DCHECK(reverse_bindings_store->IsObject() &&
         !reverse_bindings_store->IsNullOrUndefined());
  RenderFramePersistenceStore* store =
      GetOrCreateStore(GetRenderFrame(reverse_bindings_store));
  store->SetReverseBindingStore(
      isolate, v8::Local<v8::Object>::Cast(reverse_bindings_store));
}

}  // namespace api

}  // namespace electron

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  using namespace electron::api;  // NOLINT(build/namespaces)

  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.SetMethod("exposeAPIInMainWorld", &ExposeAPIInMainWorld);
  dict.SetMethod("_setReverseBindingStore", &SetReverseBindingStore);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(atom_renderer_context_bridge, Initialize)
