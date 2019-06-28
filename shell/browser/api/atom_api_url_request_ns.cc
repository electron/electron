// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/atom_api_url_request_ns.h"

#include "content/public/browser/storage_partition.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "shell/browser/api/atom_api_session.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/common/native_mate_converters/gurl_converter.h"

namespace electron {

namespace api {

namespace {

enum State {
  STATE_STARTED = 1 << 0,
  STATE_FINISHED = 1 << 1,
  STATE_CANCELED = 1 << 2,
  STATE_FAILED = 1 << 3,
  STATE_CLOSED = 1 << 4,
};

const net::NetworkTrafficAnnotationTag kAnnotation =
    net::DefineNetworkTrafficAnnotation("electron_net_module", R"(
        semantics {
          sender: "Electron Net module"
          description:
            "Issue HTTP/HTTPS requests using Chromium's native networking "
            "library."
          trigger: "Using the Net module"
          data: "Anything the user wants to send."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled."
        })");

}  // namespace

URLRequestNS::URLRequestNS(mate::Arguments* args) {
  auto request = std::make_unique<network::ResourceRequest>();
  request_ = request.get();  // ownership of |request| will be transferred.

  mate::Dictionary dict;
  if (args->GetNext(&dict)) {
    dict.Get("method", &request_->method);
    dict.Get("url", &request_->url);
  }

  std::string partition;
  mate::Handle<api::Session> session;
  if (!dict.Get("session", &session)) {
    if (dict.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else  // default session
      session = Session::FromPartition(args->isolate(), "");
  }

  auto* browser_context = session->browser_context();
  url_loader_factory_ =
      content::BrowserContext::GetDefaultStoragePartition(browser_context)
          ->GetURLLoaderFactoryForBrowserProcess();
  loader_ = network::SimpleURLLoader::Create(std::move(request), kAnnotation);

  InitWith(args->isolate(), args->GetThis());
}

URLRequestNS::~URLRequestNS() {}

bool URLRequestNS::NotStarted() const {
  return request_state_ == 0;
}

bool URLRequestNS::Finished() const {
  return request_state_ & STATE_FINISHED;
}

bool URLRequestNS::Canceled() const {
  return request_state_ & STATE_CANCELED;
}

bool URLRequestNS::Failed() const {
  return request_state_ & STATE_FAILED;
}

void URLRequestNS::Cancel() {
  // TODO(zcbenz): Implement this.
}

void URLRequestNS::Close() {
  // TODO(zcbenz): Implement this.
}

bool URLRequestNS::Write(const std::string& data, bool is_last) {
  // TODO(zcbenz): Implement this.
  return false;
}

void URLRequestNS::FollowRedirect() {
  if (request_state_ & (STATE_CANCELED | STATE_CLOSED))
    return;
  // TODO(zcbenz): Implement this.
}

bool URLRequestNS::SetExtraHeader(const std::string& name,
                                  const std::string& value) {
  if (!request_)
    return false;
  if (!net::HttpUtil::IsValidHeaderName(name))
    return false;
  if (!net::HttpUtil::IsValidHeaderValue(value))
    return false;
  request_->headers.SetHeader(name, value);
  return true;
}

void URLRequestNS::RemoveExtraHeader(const std::string& name) {
  if (request_)
    request_->headers.RemoveHeader(name);
}

void URLRequestNS::SetChunkedUpload(bool is_chunked_upload) {
  // TODO(zcbenz): Implement this.
}

void URLRequestNS::SetLoadFlags(int flags) {
  // TODO(zcbenz): Implement this.
}

void URLRequestNS::OnDataReceived(base::StringPiece string_piece,
                                  base::OnceClosure resume) {}

void URLRequestNS::OnRetry(base::OnceClosure start_retry) {}

void URLRequestNS::OnComplete(bool success) {}

void URLRequestNS::Pin() {
  if (wrapper_.IsEmpty()) {
    wrapper_.Reset(isolate(), GetWrapper());
  }
}

void URLRequestNS::Unpin() {
  wrapper_.Reset();
}

// static
mate::WrappableBase* URLRequestNS::New(mate::Arguments* args) {
  return new URLRequestNS(args);
}

// static
void URLRequestNS::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "URLRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable();
}

}  // namespace api

}  // namespace electron
