// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/asar/url_request_asar_job.h"

#include <string>

#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"

namespace asar {

URLRequestAsarJob::URLRequestAsarJob(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate,
    std::shared_ptr<Archive> archive,
    const base::FilePath& file_path,
    const Archive::FileInfo& file_info,
    const scoped_refptr<base::TaskRunner>& file_task_runner)
    : net::URLRequestJob(request, network_delegate),
      archive_(archive),
      file_path_(file_path),
      file_info_(file_info),
      stream_(new net::FileStream(file_task_runner)),
      remaining_bytes_(0),
      file_task_runner_(file_task_runner),
      weak_ptr_factory_(this) {}

URLRequestAsarJob::~URLRequestAsarJob() {}

void URLRequestAsarJob::Start() {
  remaining_bytes_ = static_cast<int64>(file_info_.size);

  int flags = base::File::FLAG_OPEN |
              base::File::FLAG_READ |
              base::File::FLAG_ASYNC;
  int rv = stream_->Open(archive_->path(), flags,
                         base::Bind(&URLRequestAsarJob::DidOpen,
                                    weak_ptr_factory_.GetWeakPtr()));
  if (rv != net::ERR_IO_PENDING)
    DidOpen(rv);
}

void URLRequestAsarJob::Kill() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  URLRequestJob::Kill();
}

bool URLRequestAsarJob::ReadRawData(net::IOBuffer* dest,
                                    int dest_size,
                                    int* bytes_read) {
  if (remaining_bytes_ < dest_size)
    dest_size = static_cast<int>(remaining_bytes_);

  // If we should copy zero bytes because |remaining_bytes_| is zero, short
  // circuit here.
  if (!dest_size) {
    *bytes_read = 0;
    return true;
  }

  int rv = stream_->Read(dest,
                         dest_size,
                         base::Bind(&URLRequestAsarJob::DidRead,
                                    weak_ptr_factory_.GetWeakPtr(),
                                    make_scoped_refptr(dest)));
  if (rv >= 0) {
    // Data is immediately available.
    *bytes_read = rv;
    remaining_bytes_ -= rv;
    DCHECK_GE(remaining_bytes_, 0);
    return true;
  }

  // Otherwise, a read error occured.  We may just need to wait...
  if (rv == net::ERR_IO_PENDING) {
    SetStatus(net::URLRequestStatus(net::URLRequestStatus::IO_PENDING, 0));
  } else {
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED, rv));
  }
  return false;
}

bool URLRequestAsarJob::GetMimeType(std::string* mime_type) const {
  return net::GetMimeTypeFromFile(file_path_, mime_type);
}

void URLRequestAsarJob::DidOpen(int result) {
  if (result != net::OK) {
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED, result));
    return;
  }

  int rv = stream_->Seek(base::File::FROM_BEGIN,
                         file_info_.offset,
                         base::Bind(&URLRequestAsarJob::DidSeek,
                                    weak_ptr_factory_.GetWeakPtr()));
  if (rv != net::ERR_IO_PENDING) {
    // stream_->Seek() failed, so pass an intentionally erroneous value
    // into DidSeek().
    DidSeek(-1);
  }
}

void URLRequestAsarJob::DidSeek(int64 result) {
  if (result != static_cast<int64>(file_info_.offset)) {
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED,
                                     net::ERR_REQUEST_RANGE_NOT_SATISFIABLE));
    return;
  }

  set_expected_content_size(remaining_bytes_);
  NotifyHeadersComplete();
}

void URLRequestAsarJob::DidRead(scoped_refptr<net::IOBuffer> buf, int result) {
  if (result > 0) {
    SetStatus(net::URLRequestStatus());  // Clear the IO_PENDING status
    remaining_bytes_ -= result;
    DCHECK_GE(remaining_bytes_, 0);
  }

  buf = NULL;

  if (result == 0) {
    NotifyDone(net::URLRequestStatus());
  } else if (result < 0) {
    NotifyDone(net::URLRequestStatus(net::URLRequestStatus::FAILED, result));
  }

  NotifyReadComplete(result);
}

}  // namespace asar
