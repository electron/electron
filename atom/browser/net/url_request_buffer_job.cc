// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_buffer_job.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"

namespace atom {

namespace {

std::string GetExtFromURL(const GURL& url) {
  std::string spec = url.spec();
  size_t index = spec.find_last_of('.');
  if (index == std::string::npos || index == spec.size())
    return std::string();
  return spec.substr(index + 1, spec.size() - index - 1);
}

void BeforeStartInUI(base::WeakPtr<URLRequestBufferJob> job,
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
                           base::BindOnce(&URLRequestBufferJob::StartAsync, job,
                                          std::move(request_options), error));
}

}  // namespace

URLRequestBufferJob::URLRequestBufferJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : net::URLRequestSimpleJob(request, network_delegate),
      status_code_(net::HTTP_NOT_IMPLEMENTED),
      weak_factory_(this) {}

URLRequestBufferJob::~URLRequestBufferJob() = default;

void URLRequestBufferJob::Start() {
  auto request_details = std::make_unique<base::DictionaryValue>();
  FillRequestDetails(request_details.get(), request());
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&JsAsker::AskForOptions, base::Unretained(isolate()),
                     handler(), std::move(request_details),
                     base::Bind(&BeforeStartInUI, weak_factory_.GetWeakPtr())));
}

void URLRequestBufferJob::StartAsync(std::unique_ptr<base::Value> options,
                                     int error) {
  if (error != net::OK) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
    return;
  }

  const base::Value* binary = nullptr;
  if (options->is_dict()) {
    base::DictionaryValue* dict =
        static_cast<base::DictionaryValue*>(options.get());
    dict->GetString("mimeType", &mime_type_);
    dict->GetString("charset", &charset_);
    dict->GetBinary("data", &binary);
  } else if (options->is_blob()) {
    binary = options.get();
  }

  if (mime_type_.empty()) {
    std::string ext = GetExtFromURL(request()->url());
#if defined(OS_WIN)
    net::GetWellKnownMimeTypeFromExtension(base::UTF8ToUTF16(ext), &mime_type_);
#else
    net::GetWellKnownMimeTypeFromExtension(ext, &mime_type_);
#endif
  }

  if (!binary) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_NOT_IMPLEMENTED));
    return;
  }

  data_ = new base::RefCountedBytes(
      reinterpret_cast<const unsigned char*>(binary->GetBlob().data()),
      binary->GetBlob().size());
  status_code_ = net::HTTP_OK;
  net::URLRequestSimpleJob::Start();
}

void URLRequestBufferJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  net::URLRequestSimpleJob::Kill();
}

void URLRequestBufferJob::GetResponseInfo(net::HttpResponseInfo* info) {
  std::string status("HTTP/1.1 ");
  status.append(base::IntToString(status_code_));
  status.append(" ");
  status.append(net::GetHttpReasonPhrase(status_code_));
  status.append("\0\0", 2);
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

int URLRequestBufferJob::GetRefCountedData(
    std::string* mime_type,
    std::string* charset,
    scoped_refptr<base::RefCountedMemory>* data,
    net::CompletionOnceCallback callback) const {
  *mime_type = mime_type_;
  *charset = charset_;
  *data = data_;
  return net::OK;
}

}  // namespace atom
