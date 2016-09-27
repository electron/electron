// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/atom_url_request.h"
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_context.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"


namespace {
const int kBufferSize = 4096;
} // namespace

namespace atom {

namespace internal {


class UploadOwnedIOBufferElementReader : public net::UploadBytesElementReader {
public:
  explicit UploadOwnedIOBufferElementReader(
    scoped_refptr<const net::IOBufferWithSize> buffer)
    : net::UploadBytesElementReader(buffer->data(), buffer->size())
    , buffer_(buffer) {
  }

  ~UploadOwnedIOBufferElementReader() override {
  }


  static UploadOwnedIOBufferElementReader* CreateWithBuffer(
    scoped_refptr<const net::IOBufferWithSize> buffer) {
    return new UploadOwnedIOBufferElementReader(std::move(buffer));
  }

private:
  scoped_refptr<const net::IOBuffer> buffer_;

  DISALLOW_COPY_AND_ASSIGN(UploadOwnedIOBufferElementReader);
};

}

AtomURLRequest::AtomURLRequest(base::WeakPtr<api::URLRequest> delegate)
  : delegate_(delegate)
  , response_read_buffer_(new net::IOBuffer(kBufferSize))
  , is_chunked_upload_(is_chunked_upload_) {
}

AtomURLRequest::~AtomURLRequest() {
}

scoped_refptr<AtomURLRequest> AtomURLRequest::Create(
    AtomBrowserContext* browser_context,
    const std::string& method,
    const std::string& url,
    base::WeakPtr<api::URLRequest> delegate) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(browser_context);
  DCHECK(!url.empty());

  auto request_context_getter = browser_context->url_request_context_getter();

  DCHECK(request_context_getter);

  auto context = request_context_getter->GetURLRequestContext();

  DCHECK(context);

  scoped_refptr<AtomURLRequest> atom_url_request = 
    new AtomURLRequest(delegate);

  atom_url_request->request_ = context->CreateRequest(GURL(url),
    net::RequestPriority::DEFAULT_PRIORITY,
    atom_url_request.get());

  atom_url_request->request_->set_method(method);
  return atom_url_request;

}


bool AtomURLRequest::Write(
    scoped_refptr<const net::IOBufferWithSize> buffer,
    bool is_last) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI); 
  return content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&AtomURLRequest::DoWriteBuffer, this, buffer, is_last));
}

void AtomURLRequest::SetChunkedUpload(bool is_chunked_upload) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // The method can be called only before switching to multi-threaded mode,
  // i.e. before the first call to write.
  // So it is safe to change the object in the UI thread.
  is_chunked_upload_ = is_chunked_upload;
}


void AtomURLRequest::Abort() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

void AtomURLRequest::SetExtraHeader(const std::string& name,
                               const std::string& value) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  request_->SetExtraRequestHeaderByName(name, value, true);
}


void AtomURLRequest::RemoveExtraHeader(const std::string& name) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  request_->RemoveRequestHeaderByName(name);
}



scoped_refptr<const net::HttpResponseHeaders>
AtomURLRequest::GetResponseHeaders() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return request_->response_headers();
}


void AtomURLRequest::PassLoginInformation(const base::string16& username,
  const base::string16& password) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (username.empty() || password.empty()) {
    content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&AtomURLRequest::DoCancelAuth, this));
  }
  else {
    content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&AtomURLRequest::DoSetAuth, this, username, password));
  }
}


void AtomURLRequest::DoWriteBuffer(
  scoped_refptr<const net::IOBufferWithSize> buffer,
  bool is_last) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

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

    if (buffer) {
      // Non-empty buffer.
      auto write_result = chunked_stream_writer_->AppendData(
        buffer->data(),
        buffer->size(),
        is_last);
    }
    else if (is_last) {
      // Empty buffer and last chunck, i.e. request.end().
      auto write_result = chunked_stream_writer_->AppendData(
        nullptr,
        0,
        true);
    }

    if (first_call) {
      request_->Start();
    }
  }
  else {

    if (buffer) {
      // Handling potential empty buffers.
      using internal::UploadOwnedIOBufferElementReader;
      auto element_reader = UploadOwnedIOBufferElementReader::CreateWithBuffer(
        std::move(buffer));
      upload_element_readers_.push_back(
        std::unique_ptr<net::UploadElementReader>(element_reader));
    }

    if (is_last) {
       auto elements_upload_data_stream = new net::ElementsUploadDataStream(
         std::move(upload_element_readers_),
         0);
      request_->set_upload(
        std::unique_ptr<net::UploadDataStream>(elements_upload_data_stream));
      request_->Start();
    }
  }
}

void AtomURLRequest::DoSetAuth(const base::string16& username,
  const base::string16& password) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  request_->SetAuth(net::AuthCredentials(username, password));
}

void AtomURLRequest::DoCancelAuth() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  request_->CancelAuth();
}

void AtomURLRequest::OnAuthRequired(net::URLRequest* request,
                    net::AuthChallengeInfo* auth_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
   
  content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegateAuthenticationRequired, 
               this,
               scoped_refptr<net::AuthChallengeInfo>(auth_info)));
}

void AtomURLRequest::OnResponseStarted(net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  DCHECK_EQ(request, request_.get());

  if (request_->status().is_success()) {
    // Cache net::HttpResponseHeaders instance, a read-only objects
    // so that headers and other http metainformation can be simultaneously
    // read from UI thread while request data is simulataneously streaming
    // on IO thread.
    response_headers_ = request_->response_headers();
  }

  content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegateResponseStarted, this));

  ReadResponse();
}

void AtomURLRequest::ReadResponse() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // Some servers may treat HEAD requests as GET requests. To free up the
  // network connection as soon as possible, signal that the request has
  // completed immediately, without trying to read any data back (all we care
  // about is the response code and headers, which we already have).
  int bytes_read = 0;
  if (request_->status().is_success() 
    /* TODO && (request_type_ != URLFetcher::HEAD)*/) {
    if (!request_->Read(response_read_buffer_.get(), kBufferSize, &bytes_read))
      bytes_read = -1; 
  }
  OnReadCompleted(request_.get(), bytes_read);
}


void AtomURLRequest::OnReadCompleted(net::URLRequest* request,
                                     int bytes_read) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  DCHECK_EQ(request, request_.get());

  do {
    if (!request_->status().is_success() || bytes_read <= 0)
      break;


    const auto result = CopyAndPostBuffer(bytes_read);
    if (!result) {
      // Failed to transfer data to UI thread.
      return;
    }
  } while (request_->Read(response_read_buffer_.get(),
                          kBufferSize,
                          &bytes_read));
    
  const auto status = request_->status();

  if (!status.is_io_pending() 
    /* TODO || request_type_ == URLFetcher::HEAD*/ ) {

    content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&AtomURLRequest::InformDelegateResponseCompleted, this));
  }

}

bool AtomURLRequest::CopyAndPostBuffer(int bytes_read) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  // data is only a wrapper for the async response_read_buffer_.
  // Make a deep copy of payload and transfer ownership to the UI thread.
  auto buffer_copy = new net::IOBufferWithSize(bytes_read);
  memcpy(buffer_copy->data(), response_read_buffer_->data(), bytes_read);

  return content::BrowserThread::PostTask(
    content::BrowserThread::UI, FROM_HERE,
    base::Bind(&AtomURLRequest::InformDelegateResponseData,
               this,
               buffer_copy));
}


void AtomURLRequest::InformDelegateAuthenticationRequired(
  scoped_refptr<net::AuthChallengeInfo> auth_info) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (delegate_) {
    delegate_->OnAuthenticationRequired(auth_info);
  }
}

void AtomURLRequest::InformDelegateResponseStarted() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (delegate_) {
    delegate_->OnResponseStarted();
  }
}

void AtomURLRequest::InformDelegateResponseData(
  scoped_refptr<net::IOBufferWithSize> data) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Transfer ownership of the data buffer, data will be released
  // by the delegate's OnResponseData.
  if (delegate_) {
    delegate_->OnResponseData(data);
  }
}

void AtomURLRequest::InformDelegateResponseCompleted() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (delegate_) {
    delegate_->OnResponseCompleted();
  }
}


}  // namespace atom