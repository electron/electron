// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/protocol_source.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/self_deleting.h"
#include "base/strings/escape.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/self_deleting_url_loader_factory.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/browser/net/asar/asar_url_loader.h"
#include "shell/common/thread_restrictions.h"
#include "url/gurl.h"

namespace electron {

namespace {

bool Fail(std::string* error, std::string message) {
  *error = std::move(message);
  return false;
}

bool ParseRoute(const base::DictValue& dict,
                ProtocolSource::Route* route,
                std::string* error) {
  const base::DictValue* source = dict.FindDict("source");
  if (!source)
    return Fail(error, "Each route needs a 'source'");
  if (const base::DictValue* match = dict.FindDict("match")) {
    if (const std::string* host = match->FindString("host"))
      route->host = base::ToLowerASCII(*host);
    if (const std::string* path = match->FindString("path")) {
      if (path->empty() || path->front() != '/')
        return Fail(error, "'match.path' must start with '/'");
      route->path_prefix = *path;
    }
  }
  if (route->path_prefix.empty() || route->path_prefix.back() != '/')
    route->path_prefix += '/';

  if (const std::string* type = source->FindString("type");
      !type || *type != "directory") {
    return Fail(error, "'source.type' must be 'directory'");
  }
  if (const std::string* root = source->FindString("root");
      root && !root->empty()) {
    route->root = base::FilePath::FromUTF8Unsafe(*root);
  } else {
    return Fail(error, "A directory source needs a 'root'");
  }
  if (!route->root.IsAbsolute())
    return Fail(error, "'source.root' must be an absolute path");
  // Requests are confined to the directory root really is, so a symlink under
  // it cannot lead elsewhere; see AsarURLLoader's containment check.
  {
    ScopedAllowBlockingForElectron allow_blocking;
    base::FilePath real_root = base::MakeAbsoluteFilePath(route->root);
    if (!real_root.empty())
      route->root = real_root;
  }
  route->root = route->root.StripTrailingSeparators();
  if (const std::string* index = source->FindString("index")) {
    if (index->find('/') != std::string::npos ||
        index->find('\\') != std::string::npos || *index == "..") {
      return Fail(error, "'source.index' must be a file name");
    }
    route->index = *index;
  } else {
    route->index = "index.html";
  }
  // Unlike file:, responses on a custom scheme carry a status line and any
  // headers the route adds.
  route->headers =
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 200 OK");
  if (const base::DictValue* headers = source->FindDict("headers")) {
    for (const auto [name, value] : *headers) {
      if (!value.is_string() || !net::HttpUtil::IsValidHeaderName(name) ||
          !net::HttpUtil::IsValidHeaderValue(value.GetString())) {
        return Fail(error, "Invalid response header '" + name + "'");
      }
      route->headers->AddHeader(name, value.GetString());
    }
  }
  return true;
}

class ProtocolSourceURLLoaderFactory
    : public network::SelfDeletingURLLoaderFactory {
 public:
  ProtocolSourceURLLoaderFactory(
      scoped_refptr<const ProtocolSource> source,
      mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver,
      base::SelfDeletingPassKey key)
      : network::SelfDeletingURLLoaderFactory(std::move(receiver), key),
        source_(std::move(source)) {}

  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> loader,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    std::optional<ProtocolSource::Match> match;
    if (request.method == "GET" || request.method == "HEAD")
      match = source_->Resolve(request.url);
    if (!match) {
      mojo::Remote<network::mojom::URLLoaderClient>(std::move(client))
          ->OnComplete(
              network::URLLoaderCompletionStatus(net::ERR_FILE_NOT_FOUND));
      return;
    }
    network::ResourceRequest file_request = request;
    file_request.url = net::FilePathToFileURL(match->file);
    asar::CreateAsarURLLoader(file_request, std::move(loader),
                              std::move(client), std::move(match->headers),
                              match->root);
  }

 private:
  ~ProtocolSourceURLLoaderFactory() override = default;

  const scoped_refptr<const ProtocolSource> source_;
};

}  // namespace

ProtocolSource::ProtocolSource() = default;
ProtocolSource::~ProtocolSource() = default;

// static
scoped_refptr<const ProtocolSource> ProtocolSource::Create(
    const base::DictValue& options,
    std::string* error) {
  const base::ListValue* routes = options.FindList("routes");
  if (!routes || routes->empty()) {
    Fail(error, "'routes' must be a non-empty array");
    return nullptr;
  }
  scoped_refptr<ProtocolSource> source(new ProtocolSource());
  for (const base::Value& item : *routes) {
    const base::DictValue* dict = item.GetIfDict();
    if (!dict) {
      Fail(error, "Each route must be an object");
      return nullptr;
    }
    Route route;
    if (!ParseRoute(*dict, &route, error))
      return nullptr;
    source->routes_.push_back(std::move(route));
  }
  // A host-specific route beats a wildcard one; then the longest path prefix.
  std::stable_sort(source->routes_.begin(), source->routes_.end(),
                   [](const Route& a, const Route& b) {
                     if (a.host.has_value() != b.host.has_value())
                       return a.host.has_value();
                     return a.path_prefix.size() > b.path_prefix.size();
                   });
  source->spec_ = options.Clone();
  return source;
}

std::optional<ProtocolSource::Match> ProtocolSource::Resolve(
    const GURL& url) const {
  if (!url.is_valid())
    return std::nullopt;
  // Non-standard schemes have no host or path; treat what follows the scheme
  // as the path.
  std::string host(url.host());
  std::string path =
      url.has_path() ? std::string(url.path()) : url.GetContent();
  if (path.empty() || path.front() != '/')
    path.insert(path.begin(), '/');
  for (const Route& route : routes_) {
    if (route.host && *route.host != host)
      continue;
    if (!base::StartsWith(path, route.path_prefix) &&
        path + '/' != route.path_prefix) {
      continue;
    }
    std::string rest = path.size() >= route.path_prefix.size()
                           ? path.substr(route.path_prefix.size())
                           : std::string();
    rest = base::UnescapeBinaryURLComponent(rest);
    if (rest.empty() || rest.back() == '/') {
      if (route.index.empty())
        return std::nullopt;
      rest += route.index;
    }
    if (rest.find('\0') != std::string::npos ||
        rest.find('\\') != std::string::npos) {
      return std::nullopt;
    }
    base::FilePath relative = base::FilePath::FromUTF8Unsafe(rest);
    if (relative.IsAbsolute() || relative.ReferencesParent())
      return std::nullopt;
    // The loader adds Content-Type etc. to what it is given; hand it a copy.
    return Match{route.root, route.root.Append(relative),
                 base::MakeRefCounted<net::HttpResponseHeaders>(
                     route.headers->raw_headers())};
  }
  return std::nullopt;
}

mojo::PendingRemote<network::mojom::URLLoaderFactory>
CreateProtocolSourceURLLoaderFactory(
    scoped_refptr<const ProtocolSource> source) {
  mojo::PendingRemote<network::mojom::URLLoaderFactory> remote;
  content::GetIOThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](scoped_refptr<const ProtocolSource> source,
             mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver) {
            base::MakeSelfDeleting<ProtocolSourceURLLoaderFactory>(
                std::move(source), std::move(receiver));
          },  // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
          std::move(source), remote.InitWithNewPipeAndPassReceiver()));
  return remote;
}

}  // namespace electron
