// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_
#define ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_

#include <string>
#include <vector>
#include "atom/browser/api/atom_api_url_request.h"
#include "atom/browser/atom_browser_context.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "net/base/auth.h"
#include "net/base/chunked_upload_data_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/upload_element_reader.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"

namespace atom {

class AtomURLRequest : public base::RefCountedThreadSafe<AtomURLRequest>,
                       public net::URLRequest::Delegate {
 public:
  static scoped_refptr<AtomURLRequest> Create(
    AtomBrowserContext* browser_context,
    const std::string& method,
    const std::string& url,
    base::WeakPtr<api::URLRequest> delegate);

  bool Write(scoped_refptr<const net::IOBufferWithSize> buffer,
                   bool is_last);
  void SetChunkedUpload(bool is_chunked_upload);
  void Cancel() const;
  void SetExtraHeader(const std::string& name, const std::string& value) const;
  void RemoveExtraHeader(const std::string& name) const;
  void PassLoginInformation(const base::string16& username,
    const base::string16& password) const;

 protected:
  // Overrides of net::URLRequest::Delegate
  void OnAuthRequired(net::URLRequest* request,
                              net::AuthChallengeInfo* auth_info) override;
  void OnResponseStarted(net::URLRequest* request) override;
  void OnReadCompleted(net::URLRequest* request,
                               int bytes_read) override;

 private:
  friend class base::RefCountedThreadSafe<AtomURLRequest>;

  explicit AtomURLRequest(base::WeakPtr<api::URLRequest> delegate);
  ~AtomURLRequest()override;

  void DoWriteBuffer(scoped_refptr<const net::IOBufferWithSize> buffer,
                     bool is_last);
  void DoCancel() const;
  void DoSetAuth(const base::string16& username,
    const base::string16& password) const;
  void DoCancelAuth() const;

  void ReadResponse();
  bool CopyAndPostBuffer(int bytes_read);

  void InformDelegateAuthenticationRequired(
    scoped_refptr<net::AuthChallengeInfo> auth_info) const;
  void InformDelegateResponseStarted(
    scoped_refptr<const net::HttpResponseHeaders>) const;
  void InformDelegateResponseData(
    scoped_refptr<net::IOBufferWithSize> data) const;
  void InformDelegateResponseCompleted() const;
  void InformDelegateRequestErrorOccured(const std::string& error) const;
  void InformDelegateResponseErrorOccured(const std::string& error) const;

  base::WeakPtr<api::URLRequest> delegate_;
  std::unique_ptr<net::URLRequest> request_;

  bool is_chunked_upload_;
  std::unique_ptr<net::ChunkedUploadDataStream> chunked_stream_;
  std::unique_ptr<net::ChunkedUploadDataStream::Writer> chunked_stream_writer_;
  std::vector<std::unique_ptr<net::UploadElementReader>>
    upload_element_readers_;
  scoped_refptr<net::IOBuffer> response_read_buffer_;

  DISALLOW_COPY_AND_ASSIGN(AtomURLRequest);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_
