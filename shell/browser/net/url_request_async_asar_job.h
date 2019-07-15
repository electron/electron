// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_
#define SHELL_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_

#include <memory>

#include "shell/browser/net/asar/url_request_asar_job.h"
#include "shell/browser/net/js_asker.h"

namespace electron {

// Like URLRequestAsarJob, but asks the JavaScript handler for file path.
class URLRequestAsyncAsarJob : public asar::URLRequestAsarJob, public JsAsker {
 public:
  URLRequestAsyncAsarJob(net::URLRequest*, net::NetworkDelegate*);
  ~URLRequestAsyncAsarJob() override;

  void StartAsync(std::unique_ptr<base::Value> options, int error);

  // URLRequestJob:
  void Start() override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void Kill() override;

 private:
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
  base::WeakPtrFactory<URLRequestAsyncAsarJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestAsyncAsarJob);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_
