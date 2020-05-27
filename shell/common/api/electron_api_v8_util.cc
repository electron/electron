// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "base/hash/hash.h"
#include "electron/buildflags/buildflags.h"
#include "shell/common/api/electron_api_key_weak_map.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/node_includes.h"
#include "url/origin.h"
#include "v8/include/v8-profiler.h"

#if BUILDFLAG(ENABLE_REMOTE_MODULE)
#include "shell/common/api/remote/remote_callback_freer.h"
#include "shell/common/api/remote/remote_object_freer.h"
#endif

namespace std {

// The hash function used by DoubleIDWeakMap.
template <typename Type1, typename Type2>
struct hash<std::pair<Type1, Type2>> {
  std::size_t operator()(std::pair<Type1, Type2> value) const {
    return base::HashInts(base::Hash(value.first), value.second);
  }
};

}  // namespace std

namespace gin {

template <typename Type1, typename Type2>
struct Converter<std::pair<Type1, Type2>> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     std::pair<Type1, Type2>* out) {
    if (!val->IsArray())
      return false;

    v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(val));
    if (array->Length() != 2)
      return false;

    auto context = isolate->GetCurrentContext();
    return Converter<Type1>::FromV8(
               isolate, array->Get(context, 0).ToLocalChecked(), &out->first) &&
           Converter<Type2>::FromV8(
               isolate, array->Get(context, 1).ToLocalChecked(), &out->second);
  }
};

}  // namespace gin

namespace {

v8::Local<v8::Value> GetHiddenValue(v8::Isolate* isolate,
                                    v8::Local<v8::Object> object,
                                    v8::Local<v8::String> key) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  v8::Local<v8::Value> value;
  v8::Maybe<bool> result = object->HasPrivate(context, privateKey);
  if (!(result.IsJust() && result.FromJust()))
    return v8::Local<v8::Value>();
  if (object->GetPrivate(context, privateKey).ToLocal(&value))
    return value;
  return v8::Local<v8::Value>();
}

void SetHiddenValue(v8::Isolate* isolate,
                    v8::Local<v8::Object> object,
                    v8::Local<v8::String> key,
                    v8::Local<v8::Value> value) {
  if (value.IsEmpty())
    return;
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  object->SetPrivate(context, privateKey, value);
}

void DeleteHiddenValue(v8::Isolate* isolate,
                       v8::Local<v8::Object> object,
                       v8::Local<v8::String> key) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Private> privateKey = v8::Private::ForApi(isolate, key);
  // Actually deleting the value would make force the object into
  // dictionary mode which is unnecessarily slow. Instead, we replace
  // the hidden value with "undefined".
  object->SetPrivate(context, privateKey, v8::Undefined(isolate));
}

int32_t GetObjectHash(v8::Local<v8::Object> object) {
  return object->GetIdentityHash();
}

void TakeHeapSnapshot(v8::Isolate* isolate) {
  isolate->GetHeapProfiler()->TakeHeapSnapshot();
}

void RequestGarbageCollectionForTesting(v8::Isolate* isolate) {
  isolate->RequestGarbageCollectionForTesting(
      v8::Isolate::GarbageCollectionType::kFullGarbageCollection);
}

bool IsSameOrigin(const GURL& l, const GURL& r) {
  return url::Origin::Create(l).IsSameOriginWith(url::Origin::Create(r));
}

#ifdef DCHECK_IS_ON
std::vector<v8::Global<v8::Value>> weakly_tracked_values;

void WeaklyTrackValue(v8::Isolate* isolate, v8::Local<v8::Value> value) {
  v8::Global<v8::Value> global_value(isolate, value);
  global_value.SetWeak();
  weakly_tracked_values.push_back(std::move(global_value));
}

void ClearWeaklyTrackedValues() {
  weakly_tracked_values.clear();
}

std::vector<v8::Local<v8::Value>> GetWeaklyTrackedValues(v8::Isolate* isolate) {
  std::vector<v8::Local<v8::Value>> locals;
  for (size_t i = 0; i < weakly_tracked_values.size(); i++) {
    if (!weakly_tracked_values[i].IsEmpty())
      locals.push_back(weakly_tracked_values[i].Get(isolate));
  }
  return locals;
}
#endif

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getHiddenValue", &GetHiddenValue);
  dict.SetMethod("setHiddenValue", &SetHiddenValue);
  dict.SetMethod("deleteHiddenValue", &DeleteHiddenValue);
  dict.SetMethod("getObjectHash", &GetObjectHash);
  dict.SetMethod("takeHeapSnapshot", &TakeHeapSnapshot);
#if BUILDFLAG(ENABLE_REMOTE_MODULE)
  dict.SetMethod("setRemoteCallbackFreer",
                 &electron::RemoteCallbackFreer::BindTo);
  dict.SetMethod("setRemoteObjectFreer", &electron::RemoteObjectFreer::BindTo);
  dict.SetMethod("addRemoteObjectRef", &electron::RemoteObjectFreer::AddRef);
  dict.SetMethod(
      "createDoubleIDWeakMap",
      &electron::api::KeyWeakMap<std::pair<std::string, int32_t>>::Create);
#endif
  dict.SetMethod("createIDWeakMap",
                 &electron::api::KeyWeakMap<int32_t>::Create);
  dict.SetMethod("requestGarbageCollectionForTesting",
                 &RequestGarbageCollectionForTesting);
  dict.SetMethod("isSameOrigin", &IsSameOrigin);
#ifdef DCHECK_IS_ON
  dict.SetMethod("getWeaklyTrackedValues", &GetWeaklyTrackedValues);
  dict.SetMethod("clearWeaklyTrackedValues", &ClearWeaklyTrackedValues);
  dict.SetMethod("weaklyTrackValue", &WeaklyTrackValue);
#endif
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_common_v8_util, Initialize)
