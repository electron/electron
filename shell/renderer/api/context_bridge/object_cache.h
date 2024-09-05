// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
#define ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_

#include <unordered_map>

#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-object.h"

namespace electron::api::context_bridge {

/**
 * NB: This is designed for context_bridge. Beware using it elsewhere!
 * Since it's a v8::Local-to-v8::Local cache, be careful to destroy it
 * before destroying the HandleScope that keeps the locals alive.
 */
class ObjectCache final {
 public:
  ObjectCache();
  ~ObjectCache();

  void CacheProxiedObject(v8::Local<v8::Value> from,
                          v8::Local<v8::Value> proxy_value);
  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(
      v8::Local<v8::Value> from) const;

 private:
  struct Hash {
    std::size_t operator()(const v8::Local<v8::Object>& obj) const {
      return obj->GetIdentityHash();
    }
  };

  // from_object ==> proxy_value
  std::unordered_map<v8::Local<v8::Object>, v8::Local<v8::Value>, Hash>
      proxy_map_;
};

}  // namespace electron::api::context_bridge

#endif  // ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
