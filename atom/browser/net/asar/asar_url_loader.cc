// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_url_loader.h"

#include "atom/common/asar/archive.h"
#include "atom/common/asar/asar_util.h"
#include "content/public/browser/file_url_loader.h"
#include "net/base/filename_util.h"

namespace asar {

void CreateAsarURLLoader(
    const network::ResourceRequest& request,
    network::mojom::URLLoaderRequest loader,
    network::mojom::URLLoaderClientPtr client,
    scoped_refptr<net::HttpResponseHeaders> extra_response_headers) {
  base::FilePath full_path;
  net::FileURLToFilePath(request.url, &full_path);

  // Determine whether it is an asar file.
  base::FilePath asar_path, relative_path;
  if (!GetAsarArchivePath(full_path, &asar_path, &relative_path)) {
    content::CreateFileURLLoader(request, std::move(loader), std::move(client),
                                 nullptr, false, extra_response_headers);
    return;
  }
}

}  // namespace asar
