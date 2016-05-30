// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include <string>

#include "atom/browser/net/js_asker.h"
#include "browser/url_request_context_getter.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace atom {

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
  int BufferCopy(net::IOBuffer* source, int num_bytes,
                 net::IOBuffer* target, int target_size);
  void ClearPendingBuffer();
  void ClearWriteBuffer();

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;

  // Saved arguments passed to ReadRawData.
  scoped_refptr<net::IOBuffer> pending_buffer_;
  int pending_buffer_size_;

  // Saved arguments passed to DataAvailable.
  scoped_refptr<net::IOBuffer> write_buffer_;
  int write_num_bytes_;
  net::CompletionCallback write_callback_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
