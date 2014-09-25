// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ASAR_ASAR_PROTOCOL_HANDLER_H_
#define ATOM_BROWSER_NET_ASAR_ASAR_PROTOCOL_HANDLER_H_

#include "atom/common/asar/archive_factory.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_request_job_factory.h"

namespace base {
class TaskRunner;
}

namespace asar {

class AsarProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  explicit AsarProtocolHandler(
      const scoped_refptr<base::TaskRunner>& file_task_runner);
  virtual ~AsarProtocolHandler();

  // net::URLRequestJobFactory::ProtocolHandler:
  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE;
  virtual bool IsSafeRedirectTarget(const GURL& location) const OVERRIDE;

 private:
  const scoped_refptr<base::TaskRunner> file_task_runner_;

  mutable ArchiveFactory archive_factory_;

  DISALLOW_COPY_AND_ASSIGN(AsarProtocolHandler);
};

}  // namespace asar

#endif  // ATOM_BROWSER_NET_ASAR_ASAR_PROTOCOL_HANDLER_H_
