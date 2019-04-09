// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_
#define ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_

#include "net/url_request/url_request_job_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace atom {

// Implementation of URLLoaderFactory.
class AtomURLLoaderFactory : public network::mojom::URLLoaderFactory {
 public:
  AtomURLLoaderFactory();
  ~AtomURLLoaderFactory() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomURLLoaderFactory);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_LOADER_FACTORY_H_
