// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_
#define ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace asar {

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers);

}  // namespace asar

#endif  // ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_
