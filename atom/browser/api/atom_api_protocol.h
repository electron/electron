// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <string>
#include <map>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context.h"

namespace base {
class DictionaryValue;
}

namespace atom {

namespace api {

class Protocol : public mate::TrackableObject<Protocol> {
 public:
  using Handler =
      base::Callback<void(const base::DictionaryValue&, v8::Local<v8::Value>)>;
  using CompletionCallback = base::Callback<void(v8::Local<v8::Value>)>;
  using BooleanCallback = base::Callback<void(bool)>;

  static mate::Handle<Protocol> Create(
      v8::Isolate* isolate, AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Protocol(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~Protocol();

 private:
  // Possible errors.
  enum ProtocolError {
    PROTOCOL_OK,  // no error
    PROTOCOL_FAIL,  // operation failed, should never occur
    PROTOCOL_REGISTERED,
    PROTOCOL_NOT_REGISTERED,
    PROTOCOL_INTERCEPTED,
    PROTOCOL_NOT_INTERCEPTED,
  };

  // The protocol handler that will create a protocol handler for certain
  // request job.
  template<typename RequestJob>
  class CustomProtocolHandler
      : public net::URLRequestJobFactory::ProtocolHandler {
   public:
    CustomProtocolHandler(
        v8::Isolate* isolate,
        net::URLRequestContextGetter* request_context,
        const Handler& handler)
        : isolate_(isolate),
          request_context_(request_context),
          handler_(handler) {}
    ~CustomProtocolHandler() override {}

    net::URLRequestJob* MaybeCreateJob(
        net::URLRequest* request,
        net::NetworkDelegate* network_delegate) const override {
      RequestJob* request_job = new RequestJob(request, network_delegate);
      request_job->SetHandlerInfo(isolate_, request_context_.get(), handler_);
      return request_job;
    }

   private:
    v8::Isolate* isolate_;
    scoped_refptr<net::URLRequestContextGetter> request_context_;
    Protocol::Handler handler_;

    DISALLOW_COPY_AND_ASSIGN(CustomProtocolHandler);
  };

  // Register schemes that can handle service worker.
  void RegisterServiceWorkerSchemes(const std::vector<std::string>& schemes);

  // Register the protocol with certain request job.
  template<typename RequestJob>
  void RegisterProtocol(const std::string& scheme,
                        const Handler& handler,
                        mate::Arguments* args) {
    CompletionCallback callback;
    args->GetNext(&callback);
    content::BrowserThread::PostTaskAndReplyWithResult(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&Protocol::RegisterProtocolInIO<RequestJob>,
                   request_context_getter_, isolate(), scheme, handler),
        base::Bind(&Protocol::OnIOCompleted,
                   GetWeakPtr(), callback));
  }
  template<typename RequestJob>
  static ProtocolError RegisterProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      v8::Isolate* isolate,
      const std::string& scheme,
      const Handler& handler) {
    auto job_factory = static_cast<AtomURLRequestJobFactory*>(
        request_context_getter->job_factory());
    if (job_factory->IsHandledProtocol(scheme))
      return PROTOCOL_REGISTERED;
    std::unique_ptr<CustomProtocolHandler<RequestJob>> protocol_handler(
        new CustomProtocolHandler<RequestJob>(
            isolate, request_context_getter.get(), handler));
    if (job_factory->SetProtocolHandler(scheme, std::move(protocol_handler)))
      return PROTOCOL_OK;
    else
      return PROTOCOL_FAIL;
  }

  // Unregister the protocol handler that handles |scheme|.
  void UnregisterProtocol(const std::string& scheme, mate::Arguments* args);
  static ProtocolError UnregisterProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Whether the protocol has handler registered.
  void IsProtocolHandled(const std::string& scheme,
                         const BooleanCallback& callback);
  static bool IsProtocolHandledInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Replace the protocol handler with a new one.
  template<typename RequestJob>
  void InterceptProtocol(const std::string& scheme,
                         const Handler& handler,
                         mate::Arguments* args) {
    CompletionCallback callback;
    args->GetNext(&callback);
    content::BrowserThread::PostTaskAndReplyWithResult(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&Protocol::InterceptProtocolInIO<RequestJob>,
                   request_context_getter_, isolate(), scheme, handler),
        base::Bind(&Protocol::OnIOCompleted,
                   GetWeakPtr(), callback));
  }
  template<typename RequestJob>
  static ProtocolError InterceptProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      v8::Isolate* isolate,
      const std::string& scheme,
      const Handler& handler) {
    auto job_factory = static_cast<AtomURLRequestJobFactory*>(
        request_context_getter->job_factory());
    if (!job_factory->IsHandledProtocol(scheme))
      return PROTOCOL_NOT_REGISTERED;
    // It is possible a protocol is handled but can not be intercepted.
    if (!job_factory->HasProtocolHandler(scheme))
      return PROTOCOL_FAIL;
    std::unique_ptr<CustomProtocolHandler<RequestJob>> protocol_handler(
        new CustomProtocolHandler<RequestJob>(
            isolate, request_context_getter.get(), handler));
    if (!job_factory->InterceptProtocol(scheme, std::move(protocol_handler)))
      return PROTOCOL_INTERCEPTED;
    return PROTOCOL_OK;
  }

  // Restore the |scheme| to its original protocol handler.
  void UninterceptProtocol(const std::string& scheme, mate::Arguments* args);
  static ProtocolError UninterceptProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Convert error code to JS exception and call the callback.
  void OnIOCompleted(const CompletionCallback& callback, ProtocolError error);

  // Convert error code to string.
  std::string ErrorCodeToString(ProtocolError error);

  AtomURLRequestJobFactory* GetJobFactoryInIO() const;

  base::WeakPtr<Protocol> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  scoped_refptr<brightray::URLRequestContextGetter> request_context_getter_;
  base::WeakPtrFactory<Protocol> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
