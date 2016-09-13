// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_async_asar_job.h"

#include <string>

#include "atom/common/atom_constants.h"

namespace atom {

URLRequestAsyncAsarJob::URLRequestAsyncAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : JsAsker<asar::URLRequestAsarJob>(request, network_delegate) {
}

void URLRequestAsyncAsarJob::StartAsync(std::unique_ptr<base::Value> options) {
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

void URLRequestAsyncAsarJob::GetResponseInfo(net::HttpResponseInfo* info) {
  std::string status("HTTP/1.1 200 OK");
  auto* headers = new net::HttpResponseHeaders(status);

  headers->AddHeader(kCORSHeader);
  info->headers = headers;
}

}  // namespace atom
