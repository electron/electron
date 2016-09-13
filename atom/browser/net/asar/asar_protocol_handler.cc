// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/asar_protocol_handler.h"

#include "atom/browser/net/asar/url_request_asar_job.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"

namespace asar {

AsarProtocolHandler::AsarProtocolHandler(
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : file_task_runner_(file_task_runner) {}

AsarProtocolHandler::~AsarProtocolHandler() {
}

net::URLRequestJob* AsarProtocolHandler::MaybeCreateJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  base::FilePath full_path;
  net::FileURLToFilePath(request->url(), &full_path);
  auto* job = new URLRequestAsarJob(request, network_delegate);
  job->Initialize(file_task_runner_, full_path);
  return job;
}

bool AsarProtocolHandler::IsSafeRedirectTarget(const GURL& location) const {
  return false;
}

}  // namespace asar
