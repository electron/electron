// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_
#define ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_

#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "v8/include/v8.h"

namespace atom {

class NodeStreamLoader {
 public:
  NodeStreamLoader(network::mojom::URLLoaderClientPtr client,
                   v8::Isolate* isolate,
                   v8::Local<v8::Value> response);
  ~NodeStreamLoader();

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeStreamLoader);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_
