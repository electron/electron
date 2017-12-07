// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_stream_job.h"

#include <algorithm>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/node_includes.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "net/base/net_errors.h"
#include "net/filter/gzip_source_stream.h"

namespace atom {

URLRequestStreamJob::URLRequestStreamJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : JsAsker<net::URLRequestJob>(request, network_delegate),
      pending_buf_(nullptr),
      pending_buf_size_(0),
      ended_(false),
      has_error_(false),
      response_headers_(nullptr),
      weak_factory_(this) {}

URLRequestStreamJob::~URLRequestStreamJob() {
  if (subscriber_) {
    content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                       std::move(subscriber_));
  }
}

void URLRequestStreamJob::BeforeStartInUI(v8::Isolate* isolate,
                                          v8::Local<v8::Value> value) {
  if (value->IsNull() || value->IsUndefined() || !value->IsObject()) {
    // Invalid opts.
    ended_ = true;
    has_error_ = true;
    return;
  }

  mate::Dictionary opts(isolate, v8::Local<v8::Object>::Cast(value));
  int status_code;
  if (!opts.Get("statusCode", &status_code)) {
    // assume HTTP OK if statusCode is not passed.
    status_code = 200;
  }
  std::string status("HTTP/1.1 ");
  status.append(base::IntToString(status_code));
  status.append(" ");
  status.append(
      net::GetHttpReasonPhrase(static_cast<net::HttpStatusCode>(status_code)));
  status.append("\0\0", 2);
  response_headers_ = new net::HttpResponseHeaders(status);

  if (opts.Get("headers", &value)) {
    mate::Converter<net::HttpResponseHeaders*>::FromV8(isolate, value,
                                                       response_headers_.get());
  }

  if (!opts.Get("data", &value)) {
    // Assume the opts is already a stream
    value = opts.GetHandle();
  } else if (value->IsNullOrUndefined()) {
    // "data" was explicitly passed as null or undefined, assume the user wants
    // to send an empty body.
    ended_ = true;
    return;
  }

  mate::Dictionary data(isolate, v8::Local<v8::Object>::Cast(value));
  if (!data.Get("on", &value) || !value->IsFunction() ||
      !data.Get("removeListener", &value) || !value->IsFunction()) {
    // If data is passed but it is not a stream, signal an error.
    ended_ = true;
    has_error_ = true;
    return;
  }

  subscriber_.reset(new mate::StreamSubscriber(isolate, data.GetHandle(),
                                              weak_factory_.GetWeakPtr()));
}

void URLRequestStreamJob::StartAsync(std::unique_ptr<base::Value> options) {
  if (has_error_) {
    OnError();
    return;
  }
  NotifyHeadersComplete();
}

void URLRequestStreamJob::OnData(std::vector<char>&& buffer) {  // NOLINT
  if (write_buffer_.empty()) {
    // Quick branch without copying.
    write_buffer_ = std::move(buffer);
  } else {
    // write_buffer_ += buffer
    size_t len = write_buffer_.size();
    write_buffer_.resize(len + buffer.size());
    std::copy(buffer.begin(), buffer.end(), write_buffer_.begin() + len);
  }

  // Copy to output.
  if (pending_buf_) {
    int len = BufferCopy(&write_buffer_, pending_buf_.get(), pending_buf_size_);
    write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + len);
    ReadRawDataComplete(len);
  }
}

void URLRequestStreamJob::OnEnd() {
  ended_ = true;
  ReadRawDataComplete(0);
}

void URLRequestStreamJob::OnError() {
  NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                         net::ERR_FAILED));
}

int URLRequestStreamJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  if (ended_)
    return 0;

  // When write_buffer_ is empty, there is no data valable yet, we have to save
  // the dest buffer util DataAvailable.
  if (write_buffer_.empty()) {
    pending_buf_ = dest;
    pending_buf_size_ = dest_size;
    return net::ERR_IO_PENDING;
  }

  // Read from the write buffer and clear them after reading.
  int len = BufferCopy(&write_buffer_, dest, dest_size);
  write_buffer_.erase(write_buffer_.begin(), write_buffer_.begin() + len);
  return len;
}

void URLRequestStreamJob::DoneReading() {
  content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                     std::move(subscriber_));
  write_buffer_.clear();
}

void URLRequestStreamJob::DoneReadingRedirectResponse() {
  DoneReading();
}

std::unique_ptr<net::SourceStream> URLRequestStreamJob::SetUpSourceStream() {
  std::unique_ptr<net::SourceStream> source =
      net::URLRequestJob::SetUpSourceStream();
  size_t i = 0;
  std::string type;
  while (response_headers_->EnumerateHeader(&i, "Content-Encoding", &type)) {
    if (base::LowerCaseEqualsASCII(type, "gzip") ||
        base::LowerCaseEqualsASCII(type, "x-gzip")) {
      return net::GzipSourceStream::Create(std::move(source),
                                           net::SourceStream::TYPE_GZIP);
    } else if (base::LowerCaseEqualsASCII(type, "deflate")) {
      return net::GzipSourceStream::Create(std::move(source),
                                           net::SourceStream::TYPE_DEFLATE);
    }
  }
  return source;
}

bool URLRequestStreamJob::GetMimeType(std::string* mime_type) const {
  return response_headers_->GetMimeType(mime_type);
}

int URLRequestStreamJob::GetResponseCode() const {
  return response_headers_->response_code();
}

void URLRequestStreamJob::GetResponseInfo(net::HttpResponseInfo* info) {
  info->headers = response_headers_;
}

int URLRequestStreamJob::BufferCopy(std::vector<char>* source,
                                    net::IOBuffer* target, int target_size) {
  int bytes_written = std::min(static_cast<int>(source->size()), target_size);
  memcpy(target->data(), source->data(), bytes_written);
  return bytes_written;
}

}  // namespace atom
