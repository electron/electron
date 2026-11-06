// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/thread_pool.h"
#include "net/base/filename_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "net/base/network_change_notifier.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/loading_params.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "shell/browser/api/electron_api_web_socket.h"
#include "shell/browser/net/resolve_host_function.h"
#include "shell/common/api/electron_api_url_loader.h"
#include "shell/common/asar/asar_util.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace {

bool IsOnline() {
  return !net::NetworkChangeNotifier::IsOffline();
}

bool IsValidHeaderName(std::string header_name) {
  return net::HttpUtil::IsValidHeaderName(header_name);
}

bool IsValidHeaderValue(std::string header_value) {
  return net::HttpUtil::IsValidHeaderValue(header_value);
}

base::FilePath FileURLToFilePath(v8::Isolate* isolate, const GURL& url) {
  base::FilePath path;
  if (!net::FileURLToFilePath(url, &path))
    gin_helper::ErrorThrower(isolate).ThrowError(
        "Failed to convert URL to file path");
  return path;
}

// What net.fetch() needs to answer a file: URL itself, worked out with the
// calls Chromium's FileURLLoader makes so the response matches the loader's.
struct FileFetchInfo {
  std::string mime_type;
  std::string last_modified;
  // The bytes the loader sends, unless the request has no body.
  std::optional<std::string> body;
};

// Runs on the thread pool. Returns nothing when the URL loader has to handle
// the URL: it fails, it is a directory or an asar archive or inside one, the
// loader would sniff its type from its contents, or the file does not fit in
// the data pipe the loader fills before anything reads the body. Otherwise
// reads the file when |read_body|, as the loader does for a file this size
// before the fetch resolves.
std::optional<FileFetchInfo> GetFileFetchInfo(const GURL& url,
                                              const bool read_body) {
  base::FilePath path;
  if (!net::FileURLToFilePath(url, &path))
    return {};

  base::FilePath asar_path, relative_path;
  if (asar::GetAsarArchivePath(path, &asar_path, &relative_path,
                               /*allow_root=*/true))
    return {};

  base::File::Info info;
  if (!base::GetFileInfo(path, &info) || info.is_directory ||
      info.size > network::GetDataPipeDefaultAllocationSize(
                      network::DataPipeAllocationSize::kLargerSizeIfPossible))
    return {};

  // The loader fails an unreadable file before it sends a response.
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                            base::File::FLAG_WIN_SHARE_DELETE);
  if (!file.IsValid())
    return {};

  std::string mime_type;
  if (!net::GetMimeTypeFromFile(base::MakeAbsoluteFilePath(path), &mime_type))
    return {};

  // The loader reads the bytes it sniffs before it responds, even when nothing
  // reads the body, and sends the first info.size bytes. A file it can't read
  // that far, or that has fewer bytes now, goes to it.
  const int64_t read_size =
      read_body ? info.size
                : std::min<int64_t>(info.size, net::kMaxBytesToSniff);
  std::string bytes(base::checked_cast<size_t>(read_size), '\0');
  if (!file.ReadAndCheck(0, base::as_writable_byte_span(bytes)))
    return {};

  return FileFetchInfo{
      std::move(mime_type), net::HttpUtil::TimeFormatHTTP(info.last_modified),
      read_body ? std::make_optional(std::move(bytes)) : std::nullopt};
}

// Resolves with { mimeType, lastModified, body }, lastModified as an HTTP
// date and body, when |read_body|, a FileFetchBody with the file's bytes, or
// with null if the URL loader has to handle the URL.
v8::Local<v8::Promise> GetFileInfoForFetch(v8::Isolate* isolate,
                                           const GURL& url,
                                           const bool read_body) {
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&GetFileFetchInfo, url, read_body),
      base::BindOnce(
          [](gin_helper::Promise<v8::Local<v8::Value>> promise,
             std::optional<FileFetchInfo> info) {
            v8::Isolate* const isolate = promise.isolate();
            v8::HandleScope handle_scope(isolate);
            if (!info) {
              promise.Resolve(v8::Null(isolate).As<v8::Value>());
              return;
            }
            auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
            dict.Set("mimeType", info->mime_type);
            dict.Set("lastModified", info->last_modified);
            if (info->body)
              dict.Set("body", electron::api::FileFetchBody::Create(
                                   isolate, std::move(*info->body)));
            promise.Resolve(dict.GetHandle());
          },
          std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> ResolveHost(
    v8::Isolate* isolate,
    std::string host,
    std::optional<network::mojom::ResolveHostParametersPtr> params) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto fn = base::MakeRefCounted<electron::ResolveHostFunction>(
      nullptr, std::move(host), params ? std::move(params.value()) : nullptr,
      base::BindOnce(
          [](gin_helper::Promise<gin_helper::Dictionary> promise,
             int64_t net_error, const std::optional<net::AddressList>& addrs) {
            if (net_error < 0) {
              promise.RejectWithErrorMessage(net::ErrorToString(net_error));
            } else {
              DCHECK(addrs.has_value() && !addrs->empty());

              v8::HandleScope handle_scope(promise.isolate());
              auto dict =
                  gin_helper::Dictionary::CreateEmpty(promise.isolate());
              dict.Set("endpoints", addrs->endpoints());
              promise.Resolve(dict);
            }
          },
          std::move(promise)));

  fn->Run();

  return handle;
}

using electron::api::SimpleURLLoaderWrapper;
using electron::api::WebSocketWrapper;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = v8::Isolate::GetCurrent();

  gin_helper::Dictionary dict{isolate, exports};
  dict.SetMethod<&IsOnline>("isOnline");
  dict.SetMethod<&IsValidHeaderName>("isValidHeaderName");
  dict.SetMethod<&IsValidHeaderValue>("isValidHeaderValue");
  dict.SetMethod<&SimpleURLLoaderWrapper::Create>("createURLLoader");
  dict.SetMethod<&WebSocketWrapper::Create>("createWebSocket");
  dict.SetMethod<&FileURLToFilePath>("fileURLToFilePath");
  dict.SetMethod<&GetFileInfoForFetch>("getFileInfoForFetch");
  dict.SetMethod<&ResolveHost>("resolveHost");
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_net, Initialize)
