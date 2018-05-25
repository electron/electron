// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_net.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/node_includes.h"
#include "base/callback.h"
#include "content/public/common/content_switches.h"
#include "native_mate/dictionary.h"

namespace atom {

namespace api {

Net::Net(v8::Isolate* isolate) {
  Init(isolate);

  net_log_ = static_cast<brightray::NetLog*>(
      atom::AtomBrowserClient::Get()->GetNetLog());
}

Net::~Net() {}

// static
v8::Local<v8::Value> Net::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new Net(isolate)).ToV8();
}

void Net::StartLogging(mate::Arguments* args) {
  if (net_log_) {
    if (args->Length() >= 1) {
      base::FilePath path;
      if (!args->GetNext(&path)) {
        args->ThrowError("Invalid file path");
        return;
      }

      if (path.empty()) {
        args->ThrowError("Empty File path");
        return;
      }

      net_log_->StartLogging(path);
      return;
    }

    net_log_->StartLogging();
  }
}

bool Net::IsLogging() {
  if (net_log_) {
    return net_log_->IsLogging();
  }
  return false;
}

void Net::StopLogging(mate::Arguments* args) {
  if (net_log_) {
    base::OnceClosure callback;
    args->GetNext(&callback);

    net_log_->StopLogging(std::move(callback));
  }
}

// static
void Net::BuildPrototype(v8::Isolate* isolate,
                         v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Net"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("URLRequest", &Net::URLRequest)
      .SetProperty("isLogging", &Net::IsLogging)
      .SetMethod("startLogging", &Net::StartLogging)
      .SetMethod("stopLogging", &Net::StopLogging);
}

v8::Local<v8::Value> Net::URLRequest(v8::Isolate* isolate) {
  return URLRequest::GetConstructor(isolate)->GetFunction();
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Net;
using atom::api::URLRequest;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();

  URLRequest::SetConstructor(isolate, base::Bind(URLRequest::New));

  mate::Dictionary dict(isolate, exports);
  dict.Set("net", Net::Create(isolate));
  dict.Set("Net", Net::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_BUILTIN_MODULE_CONTEXT_AWARE(atom_browser_net, Initialize)
