// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_
#define ATOM_BROWSER_NET_NODE_STREAM_LOADER_H_

#include <map>
#include <string>
#include <vector>

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "v8/include/v8.h"

namespace mate {
class Arguments;
}

namespace atom {

class NodeStreamLoader : public network::mojom::URLLoader {
 public:
  NodeStreamLoader(network::ResourceResponseHead head,
                   network::mojom::URLLoaderRequest loader,
                   network::mojom::URLLoaderClientPtr client,
                   v8::Isolate* isolate,
                   v8::Local<v8::Object> emitter);

 private:
  ~NodeStreamLoader() override;

  using EventCallback = base::RepeatingCallback<void(mate::Arguments* args)>;

  // URLLoader:
  void FollowRedirect(const std::vector<std::string>& removed_headers,
                      const net::HttpRequestHeaders& modified_headers,
                      const base::Optional<GURL>& new_url) override {}
  void ProceedWithResponse() override {}
  void SetPriority(net::RequestPriority priority,
                   int32_t intra_priority_value) override {}
  void PauseReadingBodyFromNet() override {}
  void ResumeReadingBodyFromNet() override {}

  // JS bindings.
  void On(const char* event, EventCallback callback);
  void OnData(mate::Arguments* args);
  void OnEnd(mate::Arguments* args);
  void OnError(mate::Arguments* args);

  // This class manages its own lifetime and should delete itself when the
  // connection is lost or finished.
  //
  // The code is updated with `content::FileURLLoader`.
  void OnConnectionError();
  void MaybeDeleteSelf();

  mojo::Binding<network::mojom::URLLoader> binding_;
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
