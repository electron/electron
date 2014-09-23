// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_
#define ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_

#include "atom/common/asar/archive.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_job.h"

namespace base {
class TaskRunner;
}

namespace asar {

class URLRequestAsarJob : public net::URLRequestJob {
 public:
  URLRequestAsarJob(net::URLRequest* request,
                    net::NetworkDelegate* network_delegate,
                    const base::FilePath& asar_path,
                    const base::FilePath& file_path,
                    const scoped_refptr<base::TaskRunner>& file_task_runner);

  // net::URLRequestJob:
  virtual void Start() OVERRIDE;
  virtual void Kill() OVERRIDE;
  virtual bool ReadRawData(net::IOBuffer* buf,
                           int buf_size,
                           int* bytes_read) OVERRIDE;
  virtual bool GetMimeType(std::string* mime_type) const OVERRIDE;

 protected:
  virtual ~URLRequestAsarJob();

 private:
  Archive archive_;
  base::FilePath file_path_;
  const scoped_refptr<base::TaskRunner> file_task_runner_;

  base::WeakPtrFactory<URLRequestAsarJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestAsarJob);
};

}  // namespace asar

#endif  // ATOM_BROWSER_NET_ASAR_URL_REQUEST_ASAR_JOB_H_
