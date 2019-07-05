// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_URL_REQUEST_STRING_JOB_H_
#define SHELL_BROWSER_NET_URL_REQUEST_STRING_JOB_H_

#include <memory>
#include <string>

#include "net/url_request/url_request_simple_job.h"
#include "shell/browser/net/js_asker.h"

namespace electron {

class URLRequestStringJob : public JsAsker, public net::URLRequestSimpleJob {
 public:
  URLRequestStringJob(net::URLRequest*, net::NetworkDelegate*);
  ~URLRequestStringJob() override;

  void StartAsync(std::unique_ptr<base::Value> options, int error);

  // URLRequestJob:
  void Start() override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void Kill() override;

  // URLRequestSimpleJob:
  int GetData(std::string* mime_type,
              std::string* charset,
              std::string* data,
              net::CompletionOnceCallback callback) const override;

 private:
  std::string mime_type_;
  std::string charset_;
  std::string data_;

  base::WeakPtrFactory<URLRequestStringJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestStringJob);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_URL_REQUEST_STRING_JOB_H_
