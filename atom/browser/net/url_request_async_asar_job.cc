// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/url_request_async_asar_job.h"

#include <string>

#include "atom/common/atom_constants.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"

namespace atom {

URLRequestAsyncAsarJob::URLRequestAsyncAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate)
    : JsAsker<asar::URLRequestAsarJob>(request, network_delegate) {}

void URLRequestAsyncAsarJob::StartAsync(std::unique_ptr<base::Value> options) {
  std::string file_path;
  if (options->is_dict()) {
    auto* path_value =
        options->FindKeyOfType("path", base::Value::Type::STRING);
    if (path_value)
      file_path = path_value->GetString();
  } else if (options->is_string()) {
    file_path = options->GetString();
  }

  if (file_path.empty()) {
    NotifyStartError(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                           net::ERR_NOT_IMPLEMENTED));
  } else {
    asar::URLRequestAsarJob::Initialize(
        base::CreateSequencedTaskRunnerWithTraits(
            {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN}),
#if defined(OS_WIN)
        base::FilePath(base::UTF8ToWide(file_path)));
#else
        base::FilePath(file_path));
#endif
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
