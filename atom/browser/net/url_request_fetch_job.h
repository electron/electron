// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_job.h"

namespace atom {

class URLRequestFetchJob : public net::URLRequestJob,
                           public net::URLFetcherDelegate {
 public:
  URLRequestFetchJob(net::URLRequest* request,
                     net::NetworkDelegate* network_delegate,
                     const GURL& url);

  base::WeakPtr<URLRequestFetchJob> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  };

  void HeadersCompleted();
  void DataAvailable(net::IOBuffer* buffer, int num_bytes);

  // net::URLRequestJob:
  void Start() override;
  void Kill() override;
  bool ReadRawData(net::IOBuffer* buf,
                   int buf_size,
                   int* bytes_read) override;
  bool GetMimeType(std::string* mime_type) const override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int GetResponseCode() const override;

  // net::URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  GURL url_;
  scoped_ptr<net::URLFetcher> fetcher_;
  std::vector<char> buffer_;
  scoped_ptr<net::HttpResponseInfo> response_info_;
  bool finished_;

  base::WeakPtrFactory<URLRequestFetchJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestFetchJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_FETCH_JOB_H_
