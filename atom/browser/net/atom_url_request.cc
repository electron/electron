// Copyright (c) 2016 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request.h"

#include <memory>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/load_flags.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/url_request/redirect_info.h"

namespace {
const int kBufferSize = 4096;
}  // namespace

namespace atom {

namespace internal {

class UploadOwnedIOBufferElementReader : public net::UploadBytesElementReader {
 public:
  explicit UploadOwnedIOBufferElementReader(
      scoped_refptr<const net::IOBufferWithSize> buffer)
      : net::UploadBytesElementReader(buffer->data(), buffer->size()),
        buffer_(buffer) {}

  ~UploadOwnedIOBufferElementReader() override {}

  static UploadOwnedIOBufferElementReader* CreateWithBuffer(
      scoped_refptr<const net::IOBufferWithSize> buffer) {
    return new UploadOwnedIOBufferElementReader(std::move(buffer));
  }

 private:
  scoped_refptr<const net::IOBuffer> buffer_;

  DISALLOW_COPY_AND_ASSIGN(UploadOwnedIOBufferElementReader);
};

}  // namespace internal

AtomURLRequest::AtomURLRequest(api::URLRequest* delegate)
    : delegate_(delegate),
      response_read_buffer_(new net::IOBuffer(kBufferSize)) {}

AtomURLRequest::~AtomURLRequest() {
  DCHECK(!request_context_getter_);
  DCHECK(!request_);
}

scoped_refptr<AtomURLRequest> AtomURLRequest::Create(
    AtomBrowserContext* browser_context,
    const std::string& method,
    const std::string& url,
    const std::string& redirect_policy,
    api::URLRequest* delegate) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(browser_context);
  DCHECK(!url.empty());
  DCHECK(delegate);
  if (!browser_context || url.empty() || !delegate) {
    return nullptr;
  }
  scoped_refptr<net::URLRequestContextGetter> request_context_getter(
      browser_context->GetRequestContext());
  DCHECK(request_context_getter);
  scoped_refptr<AtomURLRequest> atom_url_request(new AtomURLRequest(delegate));
  if (content::BrowserThread::PostTask(
          content::BrowserThread::IO, FROM_HERE,
          base::BindOnce(&AtomURLRequest::DoInitialize, atom_url_request,
                         request_context_getter, method, url,
                         redirect_policy))) {
    return atom_url_request;
  }
  return nullptr;
}

void AtomURLRequest::Terminate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  delegate_ = nullptr;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoTerminate, this));
}

void AtomURLRequest::DoInitialize(
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    const std::string& method,
    const std::string& url,
    const std::string& redirect_policy) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK(request_context_getter);

  redirect_policy_ = redirect_policy;
  request_context_getter_ = request_context_getter;
  request_context_getter_->AddObserver(this);
  auto* context = request_context_getter_->GetURLRequestContext();
  if (!context) {
    // Called after shutdown.
    DoCancelWithError("Cannot start a request after shutdown.", true);
    return;
  }

  DCHECK(context);
  request_ = context->CreateRequest(
      GURL(url), net::RequestPriority::DEFAULT_PRIORITY, this);
  if (!request_) {
    DoCancelWithError("Failed to create a net::URLRequest.", true);
    return;
  }
  request_->set_method(method);
  // Do not send cookies from the cookie store.
  DoSetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES);
  // Set a flag to stop custom protocol from intercepting this request.
  request_->SetUserData(DisableProtocolInterceptFlagKey(),
                        base::WrapUnique(new base::SupportsUserData::Data()));
}

void AtomURLRequest::DoTerminate() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  request_.reset();
  if (request_context_getter_) {
    request_context_getter_->RemoveObserver(this);
    request_context_getter_ = nullptr;
  }
}

bool AtomURLRequest::Write(scoped_refptr<const net::IOBufferWithSize> buffer,
                           bool is_last) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoWriteBuffer, this, buffer, is_last));
}

void AtomURLRequest::SetChunkedUpload(bool is_chunked_upload) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The method can be called only before switching to multi-threaded mode,
  // i.e. before the first call to write.
  // So it is safe to change the object in the UI thread.
  is_chunked_upload_ = is_chunked_upload;
}

void AtomURLRequest::Cancel() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoCancel, this));
}

void AtomURLRequest::FollowRedirect() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoFollowRedirect, this));
}

void AtomURLRequest::SetExtraHeader(const std::string& name,
                                    const std::string& value) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoSetExtraHeader, this, name, value));
}

void AtomURLRequest::RemoveExtraHeader(const std::string& name) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoRemoveExtraHeader, this, name));
}

void AtomURLRequest::PassLoginInformation(
    const base::string16& username,
    const base::string16& password) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (username.empty() || password.empty()) {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&AtomURLRequest::DoCancelAuth, this));
  } else {
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&AtomURLRequest::DoSetAuth, this, username, password));
  }
}

void AtomURLRequest::SetLoadFlags(int flags) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&AtomURLRequest::DoSetLoadFlags, this, flags));
}

void AtomURLRequest::DoWriteBuffer(
    scoped_refptr<const net::IOBufferWithSize> buffer,
    bool is_last) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }

  if (is_chunked_upload_) {
    // Chunked encoding case.

    bool first_call = false;
    if (!chunked_stream_writer_) {
      std::unique_ptr<net::ChunkedUploadDataStream> chunked_stream(
          new net::ChunkedUploadDataStream(0));
      chunked_stream_writer_ = chunked_stream->CreateWriter();
      request_->set_upload(std::move(chunked_stream));
      first_call = true;
    }

    if (buffer)
      // Non-empty buffer.
      chunked_stream_writer_->AppendData(buffer->data(), buffer->size(),
                                         is_last);
    else if (is_last)
      // Empty buffer and last chunk, i.e. request.end().
      chunked_stream_writer_->AppendData(nullptr, 0, true);

    if (first_call) {
      request_->Start();
    }
  } else {
    if (buffer) {
      // Handling potential empty buffers.
      using internal::UploadOwnedIOBufferElementReader;
      auto* element_reader =
          UploadOwnedIOBufferElementReader::CreateWithBuffer(std::move(buffer));
      upload_element_readers_.push_back(
          std::unique_ptr<net::UploadElementReader>(element_reader));
    }

    if (is_last) {
      auto* elements_upload_data_stream = new net::ElementsUploadDataStream(
          std::move(upload_element_readers_), 0);
      request_->set_upload(
          std::unique_ptr<net::UploadDataStream>(elements_upload_data_stream));
      request_->Start();
    }
  }
}

void AtomURLRequest::DoCancel() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (request_) {
    request_->Cancel();
  }
  DoTerminate();
}

void AtomURLRequest::DoFollowRedirect() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (request_ && request_->is_redirecting() && redirect_policy_ == "manual") {
    request_->FollowDeferredRedirect(
        base::nullopt /* modified_request_headers */);
  }
}

void AtomURLRequest::DoSetExtraHeader(const std::string& name,
                                      const std::string& value) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  request_->SetExtraRequestHeaderByName(name, value, true);
}

void AtomURLRequest::DoRemoveExtraHeader(const std::string& name) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  request_->RemoveRequestHeaderByName(name);
}

void AtomURLRequest::DoSetAuth(const base::string16& username,
                               const base::string16& password) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  request_->SetAuth(net::AuthCredentials(username, password));
}

void AtomURLRequest::DoCancelAuth() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  request_->CancelAuth();
}

void AtomURLRequest::DoCancelWithError(const std::string& error,
                                       bool isRequestError) {
  DoCancel();
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AtomURLRequest::InformDelegateErrorOccured, this, error,
                     isRequestError));
}

void AtomURLRequest::DoSetLoadFlags(int flags) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  request_->SetLoadFlags(request_->load_flags() | flags);
}

void AtomURLRequest::OnReceivedRedirect(net::URLRequest* request,
                                        const net::RedirectInfo& info,
                                        bool* defer_redirect) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_ || redirect_policy_ == "follow")
    return;

  if (redirect_policy_ == "error") {
    request->Cancel();
    DoCancelWithError(
        "Request cannot follow redirect with the current redirect mode", true);
  } else if (redirect_policy_ == "manual") {
    *defer_redirect = true;
    scoped_refptr<net::HttpResponseHeaders> response_headers =
        request->response_headers();
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&AtomURLRequest::InformDelegateReceivedRedirect, this,
                       info.status_code, info.new_method, info.new_url,
                       response_headers));
  }
}

void AtomURLRequest::OnAuthRequired(net::URLRequest* request,
                                    net::AuthChallengeInfo* auth_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AtomURLRequest::InformDelegateAuthenticationRequired,
                     this, scoped_refptr<net::AuthChallengeInfo>(auth_info)));
}

void AtomURLRequest::OnResponseStarted(net::URLRequest* request,
                                       int net_error) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  DCHECK_EQ(request, request_.get());

  scoped_refptr<net::HttpResponseHeaders> response_headers =
      request->response_headers();
  const auto& status = request_->status();
  if (status.is_success()) {
    // Success or pending trigger a Read.
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&AtomURLRequest::InformDelegateResponseStarted, this,
                       response_headers));
    ReadResponse();
  } else if (status.status() == net::URLRequestStatus::Status::FAILED) {
    // Report error on Start.
    DoCancelWithError(net::ErrorToString(net_error), true);
  }
  // We don't report an error is the request is canceled.
}

void AtomURLRequest::ReadResponse() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  int bytes_read = -1;
  if (request_->Read(response_read_buffer_.get(), kBufferSize, &bytes_read)) {
    OnReadCompleted(request_.get(), bytes_read);
  }
}

void AtomURLRequest::OnReadCompleted(net::URLRequest* request, int bytes_read) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request_) {
    return;
  }
  DCHECK_EQ(request, request_.get());

  const auto status = request_->status();
  if (status.error() == bytes_read &&
      bytes_read == net::ERR_CONTENT_DECODING_INIT_FAILED) {
    // When the request job is unable to create a source stream for the
    // content encoding, we fail the request.
    DoCancelWithError(net::ErrorToString(net::ERR_CONTENT_DECODING_INIT_FAILED),
                      true);
    return;
  }

  bool response_error = false;
  bool data_ended = false;
  bool data_transfer_error = false;
  do {
    if (!status.is_success()) {
      response_error = true;
      break;
    }
    if (bytes_read == 0) {
      data_ended = true;
      break;
    }
    if (bytes_read < 0 || !CopyAndPostBuffer(bytes_read)) {
      data_transfer_error = true;
      break;
    }
  } while (
      request_->Read(response_read_buffer_.get(), kBufferSize, &bytes_read));
  if (response_error) {
    DoCancelWithError(net::ErrorToString(status.ToNetError()), false);
  } else if (data_ended) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&AtomURLRequest::InformDelegateResponseCompleted, this));
    DoTerminate();
  } else if (data_transfer_error) {
    // We abort the request on corrupted data transfer.
    DoCancelWithError("Failed to transfer data from IO to UI thread.", false);
  }
}

void AtomURLRequest::OnContextShuttingDown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DoCancel();
}

bool AtomURLRequest::CopyAndPostBuffer(int bytes_read) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // data is only a wrapper for the asynchronous response_read_buffer_.
  // Make a deep copy of payload and transfer ownership to the UI thread.
  auto buffer_copy = WrapRefCounted(new net::IOBufferWithSize(bytes_read));
  memcpy(buffer_copy->data(), response_read_buffer_->data(), bytes_read);

  return content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&AtomURLRequest::InformDelegateResponseData, this,
                     buffer_copy));
}

void AtomURLRequest::InformDelegateReceivedRedirect(
    int status_code,
    const std::string& method,
    const GURL& url,
    scoped_refptr<net::HttpResponseHeaders> response_headers) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (delegate_)
    delegate_->OnReceivedRedirect(status_code, method, url, response_headers);
}

void AtomURLRequest::InformDelegateAuthenticationRequired(
    scoped_refptr<net::AuthChallengeInfo> auth_info) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (delegate_)
    delegate_->OnAuthenticationRequired(auth_info);
}

void AtomURLRequest::InformDelegateResponseStarted(
    scoped_refptr<net::HttpResponseHeaders> response_headers) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (delegate_)
    delegate_->OnResponseStarted(response_headers);
}

void AtomURLRequest::InformDelegateResponseData(
    scoped_refptr<net::IOBufferWithSize> data) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Transfer ownership of the data buffer, data will be released
  // by the delegate's OnResponseData.
  if (delegate_)
    delegate_->OnResponseData(data);
}

void AtomURLRequest::InformDelegateResponseCompleted() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (delegate_)
    delegate_->OnResponseCompleted();
}

void AtomURLRequest::InformDelegateErrorOccured(const std::string& error,
                                                bool isRequestError) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (delegate_)
    delegate_->OnError(error, isRequestError);
}

void AtomURLRequest::GetUploadProgress(mate::Dictionary* progress) const {
  net::UploadProgress upload_progress;
  if (request_) {
    progress->Set("started", true);
    upload_progress = request_->GetUploadProgress();
  } else {
    progress->Set("started", false);
  }
  progress->Set("current", upload_progress.position());
  progress->Set("total", upload_progress.size());
}

}  // namespace atom
