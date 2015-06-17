// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/io_buffer.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_fetcher_response_writer.h"
#include "net/url_request/url_request_job.h"

namespace atom {

class URLRequestFetchJob : public net::URLRequestJob,
                           public net::URLFetcherDelegate,
                           public net::URLFetcherResponseWriter {
 public:
  URLRequestFetchJob(net::URLRequest* request,
                     net::NetworkDelegate* network_delegate,
                     const GURL& url);

  // net::URLRequestJob:
  void Start() override;
  void Kill() override;
  bool ReadRawData(net::IOBuffer* buf,
                   int buf_size,
                   int* bytes_read) override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // net::URLFetchResponseWriter:
  int Initialize(const net::CompletionCallback& callback) override;
  int Write(net::IOBuffer* buffer,
            int num_bytes,
            const net::CompletionCallback& callback) override;
  int Finish(const net::CompletionCallback& callback) override;

 protected:
  virtual ~URLRequestFetchJob();

 private:
  GURL url_;
  scoped_refptr<net::DrainableIOBuffer> buffer_;

  base::WeakPtrFactory<URLRequestFetchJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
