// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_

#include <string>

#include "atom/browser/net/js_asker.h"
#include "base/memory/ref_counted_memory.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_simple_job.h"

namespace atom {

class URLRequestBufferJob : public JsAsker<net::URLRequestSimpleJob> {
 public:
  URLRequestBufferJob(net::URLRequest*, net::NetworkDelegate*);

  // JsAsker:
  void StartAsync(scoped_ptr<base::Value> options) override;

  // URLRequestJob:
  void GetResponseInfo(net::HttpResponseInfo* info) override;

  // URLRequestSimpleJob:
  int GetRefCountedData(std::string* mime_type,
                        std::string* charset,
                        scoped_refptr<base::RefCountedMemory>* data,
                        const net::CompletionCallback& callback) const override;

 private:
  std::string mime_type_;
  std::string charset_;
  scoped_refptr<base::RefCountedBytes> data_;
  net::HttpStatusCode status_code_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestBufferJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
