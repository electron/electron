// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include <string>

#include "atom/browser/net/js_asker.h"
#include "browser/url_request_context_getter.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_job.h"

namespace atom {

class AtomBrowserContext;

class URLRequestFetchJob : public JsAsker<net::URLRequestJob>,
                           public net::URLFetcherDelegate,
                           public brightray::URLRequestContextGetter::Delegate {
 public:
  URLRequestFetchJob(net::URLRequest*, net::NetworkDelegate*);

  // Called by response writer.
  void HeadersCompleted();
  int DataAvailable(net::IOBuffer* buffer,
                    int num_bytes,
                    const net::CompletionCallback& callback);

 protected:
  void StartReading();
  void ClearPendingBuffer();

  // JsAsker:
  void BeforeStartInUI(v8::Isolate*, v8::Local<v8::Value>) override;
  void StartAsync(std::unique_ptr<base::Value> options) override;

  // net::URLRequestJob:
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int GetResponseCode() const override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  scoped_refptr<net::IOBuffer> pending_buffer_;
  scoped_refptr<net::DrainableIOBuffer> drainable_buffer_;
  int pending_buffer_size_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;
  net::CompletionCallback response_piper_callback_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
