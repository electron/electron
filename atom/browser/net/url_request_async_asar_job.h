// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "atom/browser/net/js_asker.h"

namespace atom {

// Like URLRequestAsarJob, but asks the JavaScript handler for file path.
class URLRequestAsyncAsarJob : public JsAsker<asar::URLRequestAsarJob> {
 public:
  URLRequestAsyncAsarJob(net::URLRequest*, net::NetworkDelegate*);

  // JsAsker:
  void StartAsync(std::unique_ptr<base::Value> options) override;

  // URLRequestJob:
  void GetResponseInfo(net::HttpResponseInfo* info) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestAsyncAsarJob);
};

}  // namespace atom
