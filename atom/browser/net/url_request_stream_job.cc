// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <algorithm>
#include <ostream>
#include <string>

#include "atom/browser/net/url_request_stream_job.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/atom_constants.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/node_includes.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "net/filter/gzip_source_stream.h"

namespace atom {

URLRequestStreamJob::URLRequestStreamJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : JsAsker<net::URLRequestJob>(request, network_delegate),
      weak_factory_(this) {}

URLRequestStreamJob::~URLRequestStreamJob() = default;

void URLRequestStreamJob::BeforeStartInUI(v8::Isolate* isolate,
                                          v8::Local<v8::Value> value) {
  if (value->IsNull() || value->IsUndefined() || !value->IsObject()) {
    // Invalid opts.
    ended_ = true;
    errored_ = true;
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
    errored_ = true;
    return;
  }

  subscriber_.reset(new mate::EventSubscriber<URLRequestStreamJob>(
      this, isolate, data.GetHandle()));
  subscriber_->On("data", &URLRequestStreamJob::OnData);
  subscriber_->On("end", &URLRequestStreamJob::OnEnd);
  subscriber_->On("error", &URLRequestStreamJob::OnError);
}

void URLRequestStreamJob::StartAsync(std::unique_ptr<base::Value> options) {
  NotifyHeadersComplete();
}

void URLRequestStreamJob::OnData(mate::Arguments* args) {
  v8::Local<v8::Value> node_data;
  args->GetNext(&node_data);
  if (node_data->IsUint8Array()) {
    const char* data = node::Buffer::Data(node_data);
    size_t data_size = node::Buffer::Length(node_data);
    std::copy(data, data + data_size, std::back_inserter(buffer_));
  } else {
    NOTREACHED();
  }
  if (pending_io_buf_) {
    CopyMoreData(pending_io_buf_, pending_io_buf_size_);
  }
}

void URLRequestStreamJob::OnEnd(mate::Arguments* args) {
  ended_ = true;
  if (pending_io_buf_) {
    CopyMoreData(pending_io_buf_, pending_io_buf_size_);
  }
}

void URLRequestStreamJob::OnError(mate::Arguments* args) {
  errored_ = true;
  if (pending_io_buf_) {
    CopyMoreData(pending_io_buf_, pending_io_buf_size_);
  }
}

int URLRequestStreamJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&URLRequestStreamJob::CopyMoreData,
                     weak_factory_.GetWeakPtr(), WrapRefCounted(dest),
                     dest_size));
  return net::ERR_IO_PENDING;
}

void URLRequestStreamJob::DoneReading() {
  subscriber_.reset();
  buffer_.clear();
  ended_ = true;
}

void URLRequestStreamJob::DoneReadingRedirectResponse() {
  DoneReading();
}

void URLRequestStreamJob::CopyMoreDataDone(scoped_refptr<net::IOBuffer> io_buf,
                                           int status) {
  if (status <= 0) {
    subscriber_.reset();
  }
  ReadRawDataComplete(status);
  io_buf = nullptr;
}

void URLRequestStreamJob::CopyMoreData(scoped_refptr<net::IOBuffer> io_buf,
                                       int io_buf_size) {
  // reset any instance references to io_buf
  pending_io_buf_ = nullptr;
  pending_io_buf_size_ = 0;

  int read_count = 0;
  if (buffer_.size()) {
    size_t count = std::min((size_t)io_buf_size, buffer_.size());
    std::copy(buffer_.begin(), buffer_.begin() + count, io_buf->data());
    buffer_.erase(buffer_.begin(), buffer_.begin() + count);
    read_count = count;
  } else if (!ended_ && !errored_) {
    // No data available yet, save references to the IOBuffer, which will be
    // passed back to this function when OnData/OnEnd/OnError are called
    pending_io_buf_ = io_buf;
    pending_io_buf_size_ = io_buf_size;
  }

  if (!pending_io_buf_) {
    // Only call CopyMoreDataDone if we have read something.
    int status = (errored_ && !read_count) ? net::ERR_FAILED : read_count;
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&URLRequestStreamJob::CopyMoreDataDone,
                       weak_factory_.GetWeakPtr(), io_buf, status));
  }
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

}  // namespace atom
