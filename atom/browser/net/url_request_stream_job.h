// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_

#include <deque>
#include <string>

#include "atom/browser/api/event_subscriber.h"
#include "atom/browser/net/js_asker.h"
#include "base/memory/ref_counted_memory.h"
#include "native_mate/persistent_dictionary.h"
#include "net/base/io_buffer.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "v8/include/v8.h"

namespace atom {

class URLRequestStreamJob : public JsAsker<net::URLRequestJob> {
 public:
  URLRequestStreamJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate);

  void OnData(mate::Arguments* args);
  void OnEnd(mate::Arguments* args);
  void OnError(mate::Arguments* args);

  // URLRequestJob
  void GetResponseInfo(net::HttpResponseInfo* info) override;

 protected:
  // URLRequestJob
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  void DoneReading() override;
  void DoneReadingRedirectResponse() override;
  std::unique_ptr<net::SourceStream> SetUpSourceStream() override;
  bool GetMimeType(std::string* mime_type) const override;
  int GetResponseCode() const override;

 private:
  // JSAsker
  void BeforeStartInUI(v8::Isolate*, v8::Local<v8::Value>) override;
  void StartAsync(std::unique_ptr<base::Value> options) override;
  void OnResponse(bool success, std::unique_ptr<base::Value> value);

  // Callback after data is asynchronously read from the file into |buf|.
  void CopyMoreData(scoped_refptr<net::IOBuffer> io_buf, int io_buf_size);
  void CopyMoreDataDone(scoped_refptr<net::IOBuffer> io_buf, int read_count);

  std::deque<char> buffer_;
  bool ended_;
  bool errored_;
  scoped_refptr<net::IOBuffer> pending_io_buf_;
  int pending_io_buf_size_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  mate::EventSubscriber<URLRequestStreamJob>::SafePtr subscriber_;
  base::WeakPtrFactory<URLRequestStreamJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestStreamJob);
};
}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_
