// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_string_job.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"

namespace atom {

namespace {

void BeforeStartInUI(base::WeakPtr<URLRequestStringJob> job,
                     mate::Arguments* args) {
  v8::Local<v8::Value> value;
  int error = net::OK;
  std::unique_ptr<base::Value> request_options = nullptr;

  if (args->GetNext(&value)) {
    V8ValueConverter converter;
    v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
    request_options = converter.FromV8Value(value, context);
  }

  if (request_options) {
    JsAsker::IsErrorOptions(request_options.get(), &error);
  } else {
    error = net::ERR_NOT_IMPLEMENTED;
  }

  base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::IO},
                           base::BindOnce(&URLRequestStringJob::StartAsync, job,
                                          std::move(request_options), error));
}

}  // namespace

URLRequestStringJob::URLRequestStringJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : net::URLRequestSimpleJob(request, network_delegate),
      weak_factory_(this) {}

URLRequestStringJob::~URLRequestStringJob() = default;

void URLRequestStringJob::Start() {
  auto request_details = std::make_unique<base::DictionaryValue>();
  FillRequestDetails(request_details.get(), request());
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&JsAsker::AskForOptions, base::Unretained(isolate()),
                     handler(), std::move(request_details),
                     base::Bind(&BeforeStartInUI, weak_factory_.GetWeakPtr())));
}

void URLRequestStringJob::StartAsync(std::unique_ptr<base::Value> options,
                                     int error) {
  if (error != net::OK) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
    return;
  }

  if (options->is_dict()) {
    base::DictionaryValue* dict =
        static_cast<base::DictionaryValue*>(options.get());
    dict->GetString("mimeType", &mime_type_);
    dict->GetString("charset", &charset_);
    dict->GetString("data", &data_);
  } else if (options->is_string()) {
    data_ = options->GetString();
  }
  net::URLRequestSimpleJob::Start();
}

void URLRequestStringJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  net::URLRequestSimpleJob::Kill();
}

void URLRequestStringJob::GetResponseInfo(net::HttpResponseInfo* info) {
  std::string status("HTTP/1.1 200 OK");
  auto* headers = new net::HttpResponseHeaders(status);

  headers->AddHeader(kCORSHeader);

  if (!mime_type_.empty()) {
    std::string content_type_header(net::HttpRequestHeaders::kContentType);
    content_type_header.append(": ");
    content_type_header.append(mime_type_);
    headers->AddHeader(content_type_header);
  }

  info->headers = headers;
}

int URLRequestStringJob::GetData(std::string* mime_type,
                                 std::string* charset,
                                 std::string* data,
                                 net::CompletionOnceCallback callback) const {
  *mime_type = mime_type_;
  *charset = charset_;
  *data = data_;
  return net::OK;
}

}  // namespace atom
