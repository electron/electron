// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_
#define ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_

#include "base/files/file_path.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace mojo {
template <typename T>
class PendingReceiver;
template <typename T>
class PendingRemote;
}  // namespace mojo

namespace asar {

// Serves a file: request. Files inside asar archives are read by this loader;
// other files go to Chromium's file loader. With `root` set this loader reads
// both, and only files that are under that directory once symlinks and
// junctions are resolved. `root` itself must already be resolved.
void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers,
    const base::FilePath& root = base::FilePath());

}  // namespace asar

#endif  // ELECTRON_SHELL_BROWSER_NET_ASAR_ASAR_URL_LOADER_H_
