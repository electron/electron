// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_net.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/node_includes.h"
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
    base::FilePath path;
    if (!args->GetNext(&path)) {  // Prefer argument
      auto* command_line = base::CommandLine::ForCurrentProcess();
      if (command_line->HasSwitch(switches::kLogNetLog))
        path = command_line->GetSwitchValuePath(switches::kLogNetLog);
    }
    if (!path.empty()) {
      net_log_->StartLogging(path);
    }
  }
}

void Net::StopLogging() {
  if (net_log_) {
    net_log_->StopLogging();
  }
}

// static
void Net::BuildPrototype(v8::Isolate* isolate,
                         v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Net"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetProperty("URLRequest", &Net::URLRequest)
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
