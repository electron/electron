// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_async_asar_job.h"

namespace atom {

UrlRequestAsyncAsarJob::UrlRequestAsyncAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : JsAsker<asar::URLRequestAsarJob>(request, network_delegate) {
}

void UrlRequestAsyncAsarJob::StartAsync(scoped_ptr<base::Value> options) {
  base::FilePath::StringType file_path;
  if (options->IsType(base::Value::TYPE_DICTIONARY)) {
    static_cast<base::DictionaryValue*>(options.get())->GetString(
        "path", &file_path);
  } else if (options->IsType(base::Value::TYPE_STRING)) {
    options->GetAsString(&file_path);
  }

  if (file_path.empty()) {
    NotifyStartError(net::URLRequestStatus(
          net::URLRequestStatus::FAILED, net::ERR_NOT_IMPLEMENTED));
  } else {
    asar::URLRequestAsarJob::Initialize(
        content::BrowserThread::GetBlockingPool()->
            GetTaskRunnerWithShutdownBehavior(
                base::SequencedWorkerPool::SKIP_ON_SHUTDOWN),
        base::FilePath(file_path));
    asar::URLRequestAsarJob::Start();
  }
}

}  // namespace atom
