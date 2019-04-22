// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_
#define ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "v8/include/v8.h"

namespace atom {

// Old Protocol API can only serve one type of response for one scheme.
enum class ProtocolType {
  kBuffer,
  kString,
  kFile,
  kHttp,
  kStream,
  kFree,  // special type for returning arbitrary type of response.
};

using ProtocolHandler =
    base::Callback<void(const base::DictionaryValue&, v8::Local<v8::Value>)>;

// Implementation of URLLoaderFactory.
class AtomURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  AtomURLLoaderFactory(ProtocolType type, const ProtocolHandler& handler);
  ~AtomURLLoaderFactory() override;

  // network::mojom::URLLoaderFactory:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

 private:
  // TODO(zcbenz): This comes from extensions/browser/extension_protocols.cc
  // but I don't know what it actually does, find out the meanings of |Clone|
  // and |bindings_| and add comments for them.
  mojo::BindingSet<network::mojom::URLLoaderFactory> bindings_;

  ProtocolType type_;
  ProtocolHandler handler_;

  DISALLOW_COPY_AND_ASSIGN(AtomURLLoaderFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_
