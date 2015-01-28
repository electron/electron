// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_STRING_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_STRING_JOB_H_

#include "net/url_request/url_request_simple_job.h"

#include <string>

namespace atom {

class URLRequestStringJob : public net::URLRequestSimpleJob {
 public:
  URLRequestStringJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate,
                      const std::string& mime_type,
                      const std::string& charset,
                      const std::string& data);

  // URLRequestSimpleJob:
  int GetData(std::string* mime_type,
              std::string* charset,
              std::string* data,
              const net::CompletionCallback& callback) const override;

 private:
  std::string mime_type_;
  std::string charset_;
  std::string data_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestStringJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_STRING_JOB_H_
