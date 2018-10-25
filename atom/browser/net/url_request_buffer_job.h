// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_

#include <memory>
#include <string>

#include "atom/browser/net/js_asker.h"
#include "base/memory/ref_counted_memory.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_simple_job.h"

namespace atom {

class URLRequestBufferJob : public JsAsker, public net::URLRequestSimpleJob {
 public:
  URLRequestBufferJob(net::URLRequest*, net::NetworkDelegate*);
  ~URLRequestBufferJob() override;

  void StartAsync(std::unique_ptr<base::Value> options, int error);

  // URLRequestJob:
  void Start() override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  void Kill() override;

  // URLRequestSimpleJob:
  int GetRefCountedData(std::string* mime_type,
                        std::string* charset,
                        scoped_refptr<base::RefCountedMemory>* data,
                        net::CompletionOnceCallback callback) const override;

 private:
  std::string mime_type_;
  std::string charset_;
  scoped_refptr<base::RefCountedBytes> data_;
  net::HttpStatusCode status_code_;

  base::WeakPtrFactory<URLRequestBufferJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestBufferJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
