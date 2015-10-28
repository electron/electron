// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "atom/browser/net/js_asker.h"

namespace atom {

// Like URLRequestAsarJob, but asks the JavaScript handler for file path.
class UrlRequestAsyncAsarJob : public JsAsker<asar::URLRequestAsarJob> {
 public:
  UrlRequestAsyncAsarJob(net::URLRequest*, net::NetworkDelegate*);

  // JsAsker:
  void StartAsync(scoped_ptr<base::Value> options) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlRequestAsyncAsarJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_ASYNC_ASAR_JOB_H_
