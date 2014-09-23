// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/url_request_asar_job.h"

#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"

namespace asar {

URLRequestAsarJob::URLRequestAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    const base::FilePath& asar_path,
    const base::FilePath& file_path,
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : net::URLRequestJob(request, network_delegate),
      archive_(asar_path),
      file_path_(file_path),
      file_task_runner_(file_task_runner),
      weak_ptr_factory_(this) {}

URLRequestAsarJob::~URLRequestAsarJob() {}

void URLRequestAsarJob::Start() {
  Archive::FileInfo info;
  if (!archive_.Init() || !archive_.GetFileInfo(file_path_, &info)) {
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                     net::ERR_FILE_NOT_FOUND));
    return;
  }

  NotifyHeadersComplete();
}

void URLRequestAsarJob::Kill() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

bool URLRequestAsarJob::ReadRawData(net::IOBuffer* buf,
                                    int buf_size,
                                    int* bytes_read) {
  *bytes_read = 0;
  return true;
}

bool URLRequestAsarJob::GetMimeType(std::string* mime_type) const {
  return net::GetMimeTypeFromFile(file_path_, mime_type);
}

}  // namespace asar
