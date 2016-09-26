// Copyright (c) 2013 GitHub, Inc.
// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_
#define ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_

#include "base/memory/ref_counted.h"
#include "net/url_request/url_request.h"
#include "net/base/chunked_upload_data_stream.h"


namespace net {
class IOBuffer;
class IOBufferWithSize;
class DrainableIOBuffer;
};

namespace atom {

class AtomBrowserContext;

namespace api {
class URLRequest;
}

class AtomURLRequest : public base::RefCountedThreadSafe<AtomURLRequest>,
                       public net::URLRequest::Delegate {
public:
  static scoped_refptr<AtomURLRequest> Create(
    AtomBrowserContext* browser_context,
    const std::string& method,
    const std::string& url,
    base::WeakPtr<api::URLRequest> delegate);

  void WriteBuffer(scoped_refptr<const net::IOBufferWithSize> buffer, bool is_last);
  void SetChunkedUpload();
  void Abort() const;
  void SetHeader(const std::string& name, const std::string& value) const;
  std::string GetHeader(const std::string& name) const;
  void RemoveHeader(const std::string& name) const;
  void PassLoginInformation(const base::string16& username,
    const base::string16& password) const;
  scoped_refptr<const net::HttpResponseHeaders> GetResponseHeaders() const;

protected:
  // Overrides of net::URLRequest::Delegate
  virtual void OnAuthRequired(net::URLRequest* request,
                              net::AuthChallengeInfo* auth_info) override;
  virtual void OnResponseStarted(net::URLRequest* request) override;
  virtual void OnReadCompleted(net::URLRequest* request,
                               int bytes_read) override;

private:
  friend class base::RefCountedThreadSafe<AtomURLRequest>;
  void DoWriteBuffer(scoped_refptr<const net::IOBufferWithSize> buffer, bool is_last);
  void DoSetAuth(const base::string16& username,
    const base::string16& password) const;
  void DoCancelAuth() const;

  void ReadResponse();
  bool CopyAndPostBuffer(int bytes_read);

  void InformDelegateAuthenticationRequired(
    scoped_refptr<net::AuthChallengeInfo> auth_info) const;
  void InformDelegateResponseStarted() const;
  void InformDelegateResponseData(
    scoped_refptr<net::IOBufferWithSize> data) const;
  void InformDelegateResponseCompleted() const;

  AtomURLRequest(base::WeakPtr<api::URLRequest> delegate);
  virtual ~AtomURLRequest();

  base::WeakPtr<api::URLRequest> delegate_;
  std::unique_ptr<net::URLRequest> request_;

  bool is_chunked_upload_;
  std::unique_ptr<net::ChunkedUploadDataStream> chunked_stream_;
  std::unique_ptr<net::ChunkedUploadDataStream::Writer> chunked_stream_writer_;

  std::vector<std::unique_ptr<net::UploadElementReader>>upload_element_readers_;

  scoped_refptr<net::IOBuffer> response_read_buffer_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;

   DISALLOW_COPY_AND_ASSIGN(AtomURLRequest);
 };

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_URL_REQUEST_H_