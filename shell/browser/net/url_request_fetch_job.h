// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include <memory>
#include <string>

#include "atom/browser/net/js_asker.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job.h"

namespace atom {

class AtomBrowserContext;

class URLRequestFetchJob : public JsAsker,
                           public net::URLRequestJob,
                           public net::URLFetcherDelegate {
 public:
  URLRequestFetchJob(net::URLRequest*, net::NetworkDelegate*);
  ~URLRequestFetchJob() override;

  void StartAsync(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter,
      scoped_refptr<AtomBrowserContext> browser_context,
      std::unique_ptr<base::Value> options,
      int error);
  void OnError(int error);

  // Called by response writer.
  void HeadersCompleted();
  int DataAvailable(net::IOBuffer* buffer,
                    int num_bytes,
                    net::CompletionOnceCallback callback);

 protected:
  // net::URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(net::IOBuffer* buf, int buf_size) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int GetResponseCode() const override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  int BufferCopy(net::IOBuffer* source,
                 int num_bytes,
                 net::IOBuffer* target,
                 int target_size);
  void ClearPendingBuffer();
  void ClearWriteBuffer();

  scoped_refptr<AtomBrowserContext> custom_browser_context_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  std::unique_ptr<net::HttpResponseInfo> response_info_;

  // Saved arguments passed to ReadRawData.
  scoped_refptr<net::IOBuffer> pending_buffer_;
  int pending_buffer_size_ = 0;

  // Saved arguments passed to DataAvailable.
  scoped_refptr<net::IOBuffer> write_buffer_;
  int write_num_bytes_ = 0;
  net::CompletionOnceCallback write_callback_;

  base::WeakPtrFactory<URLRequestFetchJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
