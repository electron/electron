// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ADAPTER_REQUEST_JOB_H_
#define ATOM_BROWSER_NET_ADAPTER_REQUEST_JOB_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory.h"
#include "v8/include/v8.h"

namespace base {
class FilePath;
}

namespace atom {

class AtomBrowserContext;

// Ask JS which type of job it wants, and then delegate corresponding methods.
class AdapterRequestJob : public net::URLRequestJob {
 public:
  typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

  AdapterRequestJob(ProtocolHandler* protocol_handler,
                    net::URLRequest* request,
                    net::NetworkDelegate* network_delegate);

 public:
  // net::URLRequestJob:
  void Start() override;
  void Kill() override;
  bool ReadRawData(net::IOBuffer* buf,
                   int buf_size,
                   int *bytes_read) override;
  bool IsRedirectResponse(GURL* location,
                          int* http_status_code) override;
  net::Filter* SetupFilter() const override;
  bool GetMimeType(std::string* mime_type) const override;
  bool GetCharset(std::string* charset) override;
  void GetResponseInfo(net::HttpResponseInfo* info) override;
  int GetResponseCode() const override;

  base::WeakPtr<AdapterRequestJob> GetWeakPtr();

  ProtocolHandler* default_protocol_handler() { return protocol_handler_; }

  // Override this function to determine which job should be started.
  virtual void GetJobTypeInUI() = 0;

  void CreateErrorJobAndStart(int error_code);
  void CreateStringJobAndStart(const std::string& mime_type,
                               const std::string& charset,
                               const std::string& data);
  void CreateBufferJobAndStart(const std::string& mime_type,
                               const std::string& charset,
                               scoped_refptr<base::RefCountedBytes> data);
  void CreateFileJobAndStart(const base::FilePath& path);
  void CreateHttpJobAndStart(AtomBrowserContext* browser_context,
                             const GURL& url,
                             const std::string& method,
                             const std::string& referrer);
  void CreateJobFromProtocolHandlerAndStart();

 private:
  // The delegated request job.
  scoped_refptr<net::URLRequestJob> real_job_;

  // Default protocol handler.
  ProtocolHandler* protocol_handler_;

  base::WeakPtrFactory<AdapterRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AdapterRequestJob);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ADAPTER_REQUEST_JOB_H_
