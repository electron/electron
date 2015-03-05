// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/adapter_request_job.h"

#include "base/threading/sequenced_worker_pool.h"
#include "atom/browser/net/url_request_string_job.h"
#include "atom/browser/net/asar/url_request_asar_job.h"
#include "atom/common/asar/asar_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_job.h"

namespace atom {

AdapterRequestJob::AdapterRequestJob(ProtocolHandler* protocol_handler,
                                     net::URLRequest* request,
                                     net::NetworkDelegate* network_delegate)
    : URLRequestJob(request, network_delegate),
      protocol_handler_(protocol_handler),
      weak_factory_(this) {
}

void AdapterRequestJob::Start() {
  DCHECK(!real_job_.get());
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&AdapterRequestJob::GetJobTypeInUI,
                 weak_factory_.GetWeakPtr()));
}

void AdapterRequestJob::Kill() {
  if (real_job_.get())  // Kill could happen when real_job_ is created.
    real_job_->Kill();
}

bool AdapterRequestJob::ReadRawData(net::IOBuffer* buf,
                                    int buf_size,
                                    int *bytes_read) {
  DCHECK(!real_job_.get());
  return real_job_->ReadRawData(buf, buf_size, bytes_read);
}

bool AdapterRequestJob::IsRedirectResponse(GURL* location,
                                           int* http_status_code) {
  DCHECK(!real_job_.get());
  return real_job_->IsRedirectResponse(location, http_status_code);
}

net::Filter* AdapterRequestJob::SetupFilter() const {
  DCHECK(!real_job_.get());
  return real_job_->SetupFilter();
}

bool AdapterRequestJob::GetMimeType(std::string* mime_type) const {
  DCHECK(!real_job_.get());
  return real_job_->GetMimeType(mime_type);
}

bool AdapterRequestJob::GetCharset(std::string* charset) {
  DCHECK(!real_job_.get());
  return real_job_->GetCharset(charset);
}

base::WeakPtr<AdapterRequestJob> AdapterRequestJob::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void AdapterRequestJob::CreateErrorJobAndStart(int error_code) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  real_job_ = new net::URLRequestErrorJob(
      request(), network_delegate(), error_code);
  real_job_->Start();
}

void AdapterRequestJob::CreateStringJobAndStart(const std::string& mime_type,
                                                const std::string& charset,
                                                const std::string& data) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  real_job_ = new URLRequestStringJob(
      request(), network_delegate(), mime_type, charset, data);
  real_job_->Start();
}

void AdapterRequestJob::CreateFileJobAndStart(const base::FilePath& path) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  base::FilePath asar_path, relative_path;
  if (!asar::GetAsarArchivePath(path, &asar_path, &relative_path)) {
    real_job_ = new net::URLRequestFileJob(
        request(),
        network_delegate(),
        path,
        content::BrowserThread::GetBlockingPool()->
            GetTaskRunnerWithShutdownBehavior(
                base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));
  } else {
    auto archive = asar::GetOrCreateAsarArchive(asar_path);
    if (archive)
      real_job_ = new asar::URLRequestAsarJob(
          request(),
          network_delegate(),
          archive,
          relative_path,
          content::BrowserThread::GetBlockingPool()->
              GetTaskRunnerWithShutdownBehavior(
                  base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));
    else
      real_job_ = new net::URLRequestErrorJob(
          request(), network_delegate(), net::ERR_FILE_NOT_FOUND);
  }

  real_job_->Start();
}

void AdapterRequestJob::CreateJobFromProtocolHandlerAndStart() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  DCHECK(protocol_handler_);
  real_job_ = protocol_handler_->MaybeCreateJob(request(),
                                                network_delegate());
  if (!real_job_.get())
    CreateErrorJobAndStart(net::ERR_NOT_IMPLEMENTED);
  else
    real_job_->Start();
}

}  // namespace atom
