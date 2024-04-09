// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "gin/handle.h"
#include "net/base/filename_util.h"
#include "net/base/network_change_notifier.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/mojom/host_resolver.mojom.h"
#include "shell/browser/net/resolve_host_function.h"
#include "shell/common/api/electron_api_url_loader.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"
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

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  gin_helper::Dictionary dict(isolate, exports);
  dict.SetMethod("isOnline", &IsOnline);
  dict.SetMethod("isValidHeaderName", &IsValidHeaderName);
  dict.SetMethod("isValidHeaderValue", &IsValidHeaderValue);
  dict.SetMethod("createURLLoader", &SimpleURLLoaderWrapper::Create);
  dict.SetMethod("fileURLToFilePath", &FileURLToFilePath);
  dict.SetMethod("resolveHost", &ResolveHost);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_common_net, Initialize)
