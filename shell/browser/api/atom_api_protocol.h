// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_PROTOCOL_H_
#define SHELL_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context.h"
#include "shell/browser/api/trackable_object.h"
#include "shell/browser/atom_browser_context.h"
#include "shell/browser/net/atom_url_request_job_factory.h"
#include "shell/common/promise_util.h"

namespace base {
class DictionaryValue;
}

namespace electron {

namespace api {

std::vector<std::string> GetStandardSchemes();

void RegisterSchemesAsPrivileged(v8::Local<v8::Value> val,
                                 mate::Arguments* args);

class Protocol : public mate::TrackableObject<Protocol> {
 public:
  using Handler = base::RepeatingCallback<void(const base::DictionaryValue&,
                                               v8::Local<v8::Value>)>;
  using CompletionCallback =
      base::RepeatingCallback<void(v8::Local<v8::Value>)>;

  static mate::Handle<Protocol> Create(v8::Isolate* isolate,
                                       AtomBrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Protocol(v8::Isolate* isolate, AtomBrowserContext* browser_context);
  ~Protocol() override;

 private:
  // Possible errors.
  enum class ProtocolError {
    OK,    // no error
    FAIL,  // operation failed, should never occur
    REGISTERED,
    NOT_REGISTERED,
    INTERCEPTED,
    NOT_INTERCEPTED,
  };

  // The protocol handler that will create a protocol handler for certain
  // request job.
  template <typename RequestJob>
  class CustomProtocolHandler
      : public net::URLRequestJobFactory::ProtocolHandler {
   public:
    CustomProtocolHandler(v8::Isolate* isolate,
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
      request_job->SetHandlerInfo(isolate_, request_context_, handler_);
      return request_job;
    }

   private:
    v8::Isolate* isolate_;
    net::URLRequestContextGetter* request_context_;
    Protocol::Handler handler_;

    DISALLOW_COPY_AND_ASSIGN(CustomProtocolHandler);
  };

  // Register the protocol with certain request job.
  template <typename RequestJob>
  void RegisterProtocol(const std::string& scheme,
                        const Handler& handler,
                        mate::Arguments* args) {
    CompletionCallback callback;
    args->GetNext(&callback);
    auto* getter = static_cast<URLRequestContextGetter*>(
        browser_context_->GetRequestContext());
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&Protocol::RegisterProtocolInIO<RequestJob>,
                       base::RetainedRef(getter), isolate(), scheme, handler),
        base::BindOnce(&Protocol::OnIOCompleted, GetWeakPtr(), callback));
  }
  template <typename RequestJob>
  static ProtocolError RegisterProtocolInIO(
      scoped_refptr<URLRequestContextGetter> request_context_getter,
      v8::Isolate* isolate,
      const std::string& scheme,
      const Handler& handler) {
    auto* job_factory = request_context_getter->job_factory();
    if (job_factory->IsHandledProtocol(scheme))
      return ProtocolError::REGISTERED;
    auto protocol_handler = std::make_unique<CustomProtocolHandler<RequestJob>>(
        isolate, request_context_getter.get(), handler);
    if (job_factory->SetProtocolHandler(scheme, std::move(protocol_handler)))
      return ProtocolError::OK;
    else
      return ProtocolError::FAIL;
  }

  // Unregister the protocol handler that handles |scheme|.
  void UnregisterProtocol(const std::string& scheme, mate::Arguments* args);
  static ProtocolError UnregisterProtocolInIO(
      scoped_refptr<URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Whether the protocol has handler registered.
  v8::Local<v8::Promise> IsProtocolHandled(const std::string& scheme);

  // Replace the protocol handler with a new one.
  template <typename RequestJob>
  void InterceptProtocol(const std::string& scheme,
                         const Handler& handler,
                         mate::Arguments* args) {
    CompletionCallback callback;
    args->GetNext(&callback);
    auto* getter = static_cast<URLRequestContextGetter*>(
        browser_context_->GetRequestContext());
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {content::BrowserThread::IO},
        base::BindOnce(&Protocol::InterceptProtocolInIO<RequestJob>,
                       base::RetainedRef(getter), isolate(), scheme, handler),
        base::BindOnce(&Protocol::OnIOCompleted, GetWeakPtr(), callback));
  }
  template <typename RequestJob>
  static ProtocolError InterceptProtocolInIO(
      scoped_refptr<URLRequestContextGetter> request_context_getter,
      v8::Isolate* isolate,
      const std::string& scheme,
      const Handler& handler) {
    auto* job_factory = request_context_getter->job_factory();
    if (!job_factory->IsHandledProtocol(scheme))
      return ProtocolError::NOT_REGISTERED;
    // It is possible a protocol is handled but can not be intercepted.
    if (!job_factory->HasProtocolHandler(scheme))
      return ProtocolError::FAIL;
    auto protocol_handler = std::make_unique<CustomProtocolHandler<RequestJob>>(
        isolate, request_context_getter.get(), handler);
    if (!job_factory->InterceptProtocol(scheme, std::move(protocol_handler)))
      return ProtocolError::INTERCEPTED;
    return ProtocolError::OK;
  }

  // Restore the |scheme| to its original protocol handler.
  void UninterceptProtocol(const std::string& scheme, mate::Arguments* args);
  static ProtocolError UninterceptProtocolInIO(
      scoped_refptr<URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Convert error code to JS exception and call the callback.
  void OnIOCompleted(const CompletionCallback& callback, ProtocolError error);

  // Convert error code to string.
  std::string ErrorCodeToString(ProtocolError error);

  base::WeakPtr<Protocol> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  scoped_refptr<AtomBrowserContext> browser_context_;
  base::WeakPtrFactory<Protocol> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_PROTOCOL_H_
