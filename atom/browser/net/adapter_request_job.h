// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ADAPTER_REQUEST_JOB_H_
#define ATOM_BROWSER_NET_ADAPTER_REQUEST_JOB_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_job.h"
#include "net/url_request/url_request_job_factory.h"

namespace base {
class FilePath;
}

namespace atom {

// Ask JS which type of job it wants, and then delegate corresponding methods.
class AdapterRequestJob : public net::URLRequestJob {
 public:
  typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

  AdapterRequestJob(ProtocolHandler* protocol_handler,
                    net::URLRequest* request,
                    net::NetworkDelegate* network_delegate);

 public:
  // net::URLRequestJob:
  virtual void Start() OVERRIDE;
  virtual void Kill() OVERRIDE;
  virtual bool ReadRawData(net::IOBuffer* buf,
                           int buf_size,
                           int *bytes_read) OVERRIDE;
  virtual bool IsRedirectResponse(GURL* location,
                                  int* http_status_code) OVERRIDE;
  virtual net::Filter* SetupFilter() const OVERRIDE;
  virtual bool GetMimeType(std::string* mime_type) const OVERRIDE;
  virtual bool GetCharset(std::string* charset) OVERRIDE;

  base::WeakPtr<AdapterRequestJob> GetWeakPtr();

  ProtocolHandler* default_protocol_handler() { return protocol_handler_; }

  // Override this function to determine which job should be started.
  virtual void GetJobTypeInUI() = 0;

  void CreateErrorJobAndStart(int error_code);
  void CreateStringJobAndStart(const std::string& mime_type,
                               const std::string& charset,
                               const std::string& data);
  void CreateFileJobAndStart(const base::FilePath& path);
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
