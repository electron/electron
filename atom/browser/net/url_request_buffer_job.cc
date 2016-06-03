// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_buffer_job.h"

#include <string>

#include "atom/common/atom_constants.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
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

}  // namespace

URLRequestBufferJob::URLRequestBufferJob(
    net::URLRequest* request, net::NetworkDelegate* network_delegate)
    : JsAsker<net::URLRequestSimpleJob>(request, network_delegate),
      status_code_(net::HTTP_NOT_IMPLEMENTED) {
}

void URLRequestBufferJob::StartAsync(std::unique_ptr<base::Value> options) {
  const base::BinaryValue* binary = nullptr;
  if (options->IsType(base::Value::TYPE_DICTIONARY)) {
    base::DictionaryValue* dict =
        static_cast<base::DictionaryValue*>(options.get());
    dict->GetString("mimeType", &mime_type_);
    dict->GetString("charset", &charset_);
    dict->GetBinary("data", &binary);
  } else if (options->IsType(base::Value::TYPE_BINARY)) {
    options->GetAsBinary(&binary);
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
    NotifyStartError(net::URLRequestStatus(
          net::URLRequestStatus::FAILED, net::ERR_NOT_IMPLEMENTED));
    return;
  }

  data_ = new base::RefCountedBytes(
      reinterpret_cast<const unsigned char*>(binary->GetBuffer()),
      binary->GetSize());
  status_code_ = net::HTTP_OK;
  net::URLRequestSimpleJob::Start();
}

void URLRequestBufferJob::GetResponseInfo(net::HttpResponseInfo* info) {
  std::string status("HTTP/1.1 ");
  status.append(base::IntToString(status_code_));
  status.append(" ");
  status.append(net::GetHttpReasonPhrase(status_code_));
  status.append("\0\0", 2);
  net::HttpResponseHeaders* headers = new net::HttpResponseHeaders(status);

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
    const net::CompletionCallback& callback) const {
  *mime_type = mime_type_;
  *charset = charset_;
  *data = data_;
  return net::OK;
}

}  // namespace atom
