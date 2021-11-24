// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
#define ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_

#include <forward_list>
#include <unordered_map>
#include <utility>

#include "base/containers/linked_list.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "shell/renderer/electron_render_frame_observer.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace electron {

namespace api {

namespace context_bridge {

using ObjectCachePair = std::pair<v8::Local<v8::Value>, v8::Local<v8::Value>>;

class ObjectCache final {
 public:
  ObjectCache();
  ~ObjectCache();

  void CacheProxiedObject(v8::Local<v8::Value> from,
                          v8::Local<v8::Value> proxy_value);
  v8::MaybeLocal<v8::Value> GetCachedProxiedObject(
      v8::Local<v8::Value> from) const;

 private:
  // object_identity ==> [from_value, proxy_value]
  std::unordered_map<int, std::forward_list<ObjectCachePair>> proxy_map_;
};

}  // namespace context_bridge

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_RENDERER_API_CONTEXT_BRIDGE_OBJECT_CACHE_H_
