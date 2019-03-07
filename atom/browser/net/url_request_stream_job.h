// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/stream_subscriber.h"
#include "atom/browser/net/js_asker.h"
#include "base/memory/scoped_refptr.h"
#include "net/base/io_buffer.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_job.h"

namespace atom {

class URLRequestStreamJob : public JsAsker, public net::URLRequestJob {
 public:
  URLRequestStreamJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate);
  ~URLRequestStreamJob() override;

  void StartAsync(scoped_refptr<mate::StreamSubscriber> subscriber,
                  scoped_refptr<net::HttpResponseHeaders> response_headers,
                  bool ended,
                  int error);

  void OnData(std::vector<char>&& buffer);  // NOLINT
  void OnEnd();
  void OnError(int error);

 protected:
  // URLRequestJob
  void Start() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  void DoneReading() override;
  void DoneReadingRedirectResponse() override;
  std::unique_ptr<net::SourceStream> SetUpSourceStream() override;
  bool GetMimeType(std::string* mime_type) const override;
  int GetResponseCode() const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void GetLoadTimingInfo(net::LoadTimingInfo* load_timing_info) const override;
  void Kill() override;

 private:
  int BufferCopy(std::vector<char>* source,
                 net::IOBuffer* target,
                 int target_size);

  // Saved arguments passed to ReadRawData.
  scoped_refptr<net::IOBuffer> pending_buf_;
  int pending_buf_size_;

  // Saved arguments passed to OnData.
  std::vector<char> write_buffer_;

  bool ended_;
  base::TimeTicks request_start_time_;
  base::TimeTicks response_start_time_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  scoped_refptr<mate::StreamSubscriber> subscriber_;

  base::WeakPtrFactory<URLRequestStreamJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestStreamJob);
};
}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_STREAM_JOB_H_
