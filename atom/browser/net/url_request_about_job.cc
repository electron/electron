// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_about_job.h"

#include "base/threading/thread_task_runner_handle.h"

namespace atom {

URLRequestAboutJob::URLRequestAboutJob(net::URLRequest* request,
                                       net::NetworkDelegate* network_delegate)
    : net::URLRequestJob(request, network_delegate), weak_ptr_factory_(this) {}

void URLRequestAboutJob::Start() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&URLRequestAboutJob::StartAsync,
                            weak_ptr_factory_.GetWeakPtr()));
}

void URLRequestAboutJob::Kill() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

bool URLRequestAboutJob::GetMimeType(std::string* mime_type) const {
  *mime_type = "text/html";
  return true;
}

URLRequestAboutJob::~URLRequestAboutJob() {}

void URLRequestAboutJob::StartAsync() {
  NotifyHeadersComplete();
}

}  // namespace atom
