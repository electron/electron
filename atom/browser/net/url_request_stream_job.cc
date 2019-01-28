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
#include "base/task/post_task.h"
#include "base/time/time.h"
#include "content/public/browser/browser_task_traits.h"
#include "native_mate/dictionary.h"
#include "net/base/net_errors.h"
#include "net/filter/gzip_source_stream.h"

namespace atom {

namespace {

void BeforeStartInUI(base::WeakPtr<URLRequestStreamJob> job,
                     mate::Arguments* args) {
  v8::Local<v8::Value> value;
  int error = net::OK;
  bool ended = false;
  if (!args->GetNext(&value) || !value->IsObject()) {
    // Invalid opts.
    base::PostTaskWithTraits(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&URLRequestStreamJob::OnError, job, net::ERR_FAILED));
    return;
  }

  mate::Dictionary opts(args->isolate(), v8::Local<v8::Object>::Cast(value));
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
  scoped_refptr<net::HttpResponseHeaders> response_headers(
      new net::HttpResponseHeaders(status));

  if (opts.Get("headers", &value)) {
    mate::Converter<net::HttpResponseHeaders*>::FromV8(args->isolate(), value,
                                                       response_headers.get());
  }

  if (!opts.Get("data", &value)) {
    // Assume the opts is already a stream
    value = opts.GetHandle();
  } else if (value->IsNullOrUndefined()) {
    // "data" was explicitly passed as null or undefined, assume the user wants
    // to send an empty body.
    ended = true;
    base::PostTaskWithTraits(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&URLRequestStreamJob::StartAsync, job, nullptr,
                       base::RetainedRef(response_headers), ended, error));
    return;
  }

  mate::Dictionary data(args->isolate(), v8::Local<v8::Object>::Cast(value));
  if (!data.Get("on", &value) || !value->IsFunction() ||
      !data.Get("removeListener", &value) || !value->IsFunction()) {
    // If data is passed but it is not a stream, signal an error.
    base::PostTaskWithTraits(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&URLRequestStreamJob::OnError, job, net::ERR_FAILED));
    return;
  }

  auto subscriber = std::make_unique<mate::StreamSubscriber>(
      args->isolate(), data.GetHandle(), job);

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&URLRequestStreamJob::StartAsync, job,
                     std::move(subscriber), base::RetainedRef(response_headers),
                     ended, error));
}

}  // namespace

URLRequestStreamJob::URLRequestStreamJob(net::URLRequest* request,
                                         net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate),
      pending_buf_(nullptr),
      pending_buf_size_(0),
      ended_(false),
      response_headers_(nullptr),
      weak_factory_(this) {}

URLRequestStreamJob::~URLRequestStreamJob() {
  if (subscriber_) {
    content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                       std::move(subscriber_));
  }
}

void URLRequestStreamJob::Start() {
  auto request_details = std::make_unique<base::DictionaryValue>();
  FillRequestDetails(request_details.get(), request());
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&JsAsker::AskForOptions, base::Unretained(isolate()),
                     handler(), std::move(request_details),
                     base::Bind(&BeforeStartInUI, weak_factory_.GetWeakPtr())));
}

void URLRequestStreamJob::StartAsync(
    std::unique_ptr<mate::StreamSubscriber> subscriber,
    scoped_refptr<net::HttpResponseHeaders> response_headers,
    bool ended,
    int error) {
  if (error != net::OK) {
    NotifyStartError(
        net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
    return;
  }

  ended_ = ended;
  response_headers_ = response_headers;
  subscriber_ = std::move(subscriber);
  request_start_time_ = base::TimeTicks::Now();
  NotifyHeadersComplete();
}

void URLRequestStreamJob::OnData(std::vector<char>&& buffer) {  // NOLINT
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
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
    pending_buf_ = nullptr;
    pending_buf_size_ = 0;
    ReadRawDataComplete(len);
  }
}

void URLRequestStreamJob::OnEnd() {
  ended_ = true;
  if (pending_buf_) {
    ReadRawDataComplete(0);
  }
}

void URLRequestStreamJob::OnError(int error) {
  NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED, error));
}

int URLRequestStreamJob::ReadRawData(net::IOBuffer* dest, int dest_size) {
  response_start_time_ = base::TimeTicks::Now();

  if (ended_ && write_buffer_.empty())
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

void URLRequestStreamJob::GetLoadTimingInfo(
    net::LoadTimingInfo* load_timing_info) const {
  load_timing_info->send_start = request_start_time_;
  load_timing_info->send_end = request_start_time_;
  load_timing_info->request_start = request_start_time_;
  load_timing_info->receive_headers_end = response_start_time_;
}

void URLRequestStreamJob::Kill() {
  weak_factory_.InvalidateWeakPtrs();
  net::URLRequestJob::Kill();
}

int URLRequestStreamJob::BufferCopy(std::vector<char>* source,
                                    net::IOBuffer* target,
                                    int target_size) {
  int bytes_written = std::min(static_cast<int>(source->size()), target_size);
  memcpy(target->data(), source->data(), bytes_written);
  return bytes_written;
}

}  // namespace atom
