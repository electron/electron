// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_STRING_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_STRING_JOB_H_

#include <string>

#include "atom/browser/net/js_asker.h"
#include "net/url_request/url_request_simple_job.h"

namespace atom {

class URLRequestStringJob : public JsAsker<net::URLRequestSimpleJob> {
 public:
  URLRequestStringJob(net::URLRequest*, net::NetworkDelegate*);

  // JsAsker:
  void StartAsync(std::unique_ptr<base::Value> options) override;

  // URLRequestJob:
  void GetResponseInfo(net::HttpResponseInfo* info) override;

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
