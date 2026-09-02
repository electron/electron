// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_PROTOCOL_SOURCE_H_
#define ELECTRON_SHELL_BROWSER_NET_PROTOCOL_SOURCE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"

class GURL;

namespace electron {

// What protocol.registerSource() installs for a scheme: an immutable list of
// routes, each mapping URLs on the scheme to a directory on disk. Built on the
// UI thread, read on the IO thread.
class ProtocolSource : public base::RefCountedThreadSafe<ProtocolSource> {
 public:
  struct Route {
    Route();
    Route(Route&&);
    Route& operator=(Route&&);
    ~Route();
    std::optional<std::string> host;  // nullopt matches any host
    std::string path_prefix;          // always starts and ends with '/'
    base::FilePath root;
    std::string index;  // served for a path ending in '/', may be empty
    scoped_refptr<net::HttpResponseHeaders> headers;
  };

  // Parses registerSource()'s options; null with `error` set on failure.
  static scoped_refptr<const ProtocolSource> Create(
      const base::DictValue& options,
      std::string* error);

  // The file `url` maps to and the headers to send with it, or nullopt when
  // no route matches or the path escapes the route's root.
  struct Match {
    base::FilePath file;
    scoped_refptr<net::HttpResponseHeaders> headers;
  };
  std::optional<Match> Resolve(const GURL& url) const;

  const base::DictValue& spec() const { return spec_; }

 private:
  friend class base::RefCountedThreadSafe<ProtocolSource>;
  ProtocolSource();
  ~ProtocolSource();

  std::vector<Route> routes_;  // most specific first
  base::DictValue spec_;       // what getSource() returns
};

// A URLLoaderFactory, bound on the IO thread, that serves `source`.
mojo::PendingRemote<network::mojom::URLLoaderFactory>
CreateProtocolSourceURLLoaderFactory(
    scoped_refptr<const ProtocolSource> source);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_PROTOCOL_SOURCE_H_
