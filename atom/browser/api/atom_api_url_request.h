// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_
#define ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_

#include "atom/browser/api/trackable_object.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context.h"

namespace atom {

class AtomURLRequest;

namespace api {

class URLRequest : public mate::EventEmitter<URLRequest> {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
    v8::Local<v8::FunctionTemplate> prototype);

  void start();
  void stop();
  void OnResponseStarted();
 protected:
  URLRequest(v8::Isolate* isolate, 
             v8::Local<v8::Object> wrapper);
  ~URLRequest() override;



 private:
  void pin();
  void unpin();

  scoped_refptr<AtomURLRequest> atom_url_request_;
  v8::Global<v8::Object> wrapper_;
  base::WeakPtrFactory<URLRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequest);
};

}  // namepsace api

}  // namepsace atom

#endif  // ATOM_BROWSER_API_ATOM_API_URL_REQUEST_H_