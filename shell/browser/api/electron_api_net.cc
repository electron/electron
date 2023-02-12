// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "gin/handle.h"
#include "net/base/filename_util.h"
#include "net/base/network_change_notifier.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/features.h"
#include "shell/browser/api/electron_api_url_loader.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/object_template_builder.h"

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
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_net, Initialize)
