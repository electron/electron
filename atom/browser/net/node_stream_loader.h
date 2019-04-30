// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_
#define ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_

#include <map>
#include <string>

#include "services/network/public/mojom/url_loader.mojom.h"
#include "v8/include/v8.h"

namespace mate {
class Arguments;
}

namespace atom {

class NodeStreamLoader {
 public:
  NodeStreamLoader(network::ResourceResponseHead head,
                   network::mojom::URLLoaderClientPtr client,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> emitter);

 private:
  ~NodeStreamLoader();

  using EventCallback = base::RepeatingCallback<void(mate::Arguments* args)>;

  void On(const char* event, EventCallback callback);

  void OnData(mate::Arguments* args);
  void OnEnd(mate::Arguments* args);
  void OnError(mate::Arguments* args);

  network::mojom::URLLoaderClientPtr client_;

  v8::Isolate* isolate_;
  v8::Global<v8::Object> emitter_;

  // Pipes for communicating between Node and NetworkService.
  mojo::ScopedDataPipeProducerHandle producer_;

  // Store the V8 callbacks to unsubscribe them later.
  std::map<std::string, v8::Global<v8::Value>> handlers_;

  base::WeakPtrFactory<NodeStreamLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NodeStreamLoader);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_
