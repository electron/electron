// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/adapter_request_job.h"

#include "atom/browser/atom_browser_context.h"
#include "base/threading/sequenced_worker_pool.h"
#include "atom/browser/net/url_request_buffer_job.h"
#include "atom/browser/net/url_request_fetch_job.h"
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

void AdapterRequestJob::GetResponseInfo(net::HttpResponseInfo* info) {
  real_job_->GetResponseInfo(info);
}

int AdapterRequestJob::GetResponseCode() const {
  return real_job_->GetResponseCode();
}

base::WeakPtr<AdapterRequestJob> AdapterRequestJob::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void AdapterRequestJob::CreateErrorJobAndStart(int error_code) {
  real_job_ = new net::URLRequestErrorJob(
      request(), network_delegate(), error_code);
  real_job_->Start();
}

void AdapterRequestJob::CreateStringJobAndStart(const std::string& mime_type,
                                                const std::string& charset,
                                                const std::string& data) {
  real_job_ = new URLRequestStringJob(
      request(), network_delegate(), mime_type, charset, data);
  real_job_->Start();
}

void AdapterRequestJob::CreateBufferJobAndStart(
    const std::string& mime_type,
    const std::string& charset,
    scoped_refptr<base::RefCountedBytes> data) {
  real_job_ = new URLRequestBufferJob(
      request(), network_delegate(), mime_type, charset, data);
  real_job_->Start();
}

void AdapterRequestJob::CreateFileJobAndStart(const base::FilePath& path) {
  real_job_ = asar::CreateJobFromPath(
      path,
      request(),
      network_delegate(),
      content::BrowserThread::GetBlockingPool()->
          GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));
  real_job_->Start();
}

void AdapterRequestJob::CreateHttpJobAndStart(
    AtomBrowserContext* browser_context,
    const GURL& url,
    const std::string& method,
    const std::string& referrer) {
  if (!url.is_valid()) {
    CreateErrorJobAndStart(net::ERR_INVALID_URL);
    return;
  }

  real_job_ = new URLRequestFetchJob(browser_context, request(),
                                     network_delegate(), url, method, referrer);
  real_job_->Start();
}

void AdapterRequestJob::CreateJobFromProtocolHandlerAndStart() {
  real_job_ = protocol_handler_->MaybeCreateJob(request(),
                                                network_delegate());
  if (!real_job_.get())
    CreateErrorJobAndStart(net::ERR_NOT_IMPLEMENTED);
  else
    real_job_->Start();
}

}  // namespace atom
