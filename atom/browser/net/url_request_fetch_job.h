// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include <string>

#include "atom/browser/net/js_asker.h"
#include "brightray/browser/url_request_context_getter.h"
#include "content/browser/streams/stream.h"
#include "content/browser/streams/stream_read_observer.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace atom {

class URLRequestFetchJob : public JsAsker<net::URLRequestJob>,
                           public net::URLFetcherDelegate,
                           public brightray::URLRequestContextGetter::Delegate,
                           public content::StreamReadObserver {
 public:
  URLRequestFetchJob(net::URLRequest*, net::NetworkDelegate*);

  // Called by response writer.
  void HeadersCompleted();

  content::Stream* stream() const { return stream_.get(); }

 protected:
  // JsAsker:
  void BeforeStartInUI(v8::Isolate*, v8::Local<v8::Value>) override;
  void StartAsync(std::unique_ptr<base::Value> options) override;

  // net::URLRequestJob:
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int GetResponseCode() const override;
  int64_t GetTotalReceivedBytes() const override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // content::StreamReadObserver:
  void OnDataAvailable(content::Stream* stream) override;

 private:
  void ClearStream();

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;
  scoped_refptr<content::Stream> stream_;

  // Saved arguments passed to ReadRawData.
  scoped_refptr<net::IOBuffer> pending_buffer_;
  int pending_buffer_size_;
  int total_bytes_read_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
