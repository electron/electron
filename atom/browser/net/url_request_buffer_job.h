// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
#define ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "net/url_request/url_request_simple_job.h"
#include "atom/common/node_includes.h"

namespace atom {

class URLRequestBufferJob : public net::URLRequestSimpleJob {
 public:
  URLRequestBufferJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate,
                      const std::string& mime_type,
                      const std::string& charset,
                      scoped_refptr<base::RefCountedBytes> data);

  // URLRequestSimpleJob:
  int GetRefCountedData(std::string* mime_type,
                        std::string* charset,
                        scoped_refptr<base::RefCountedMemory>* data,
                        const net::CompletionCallback& callback) const override;

 private:
  std::string mime_type_;
  std::string charset_;
  scoped_refptr<base::RefCountedBytes> buffer_data_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestBufferJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_URL_REQUEST_BUFFER_JOB_H_
