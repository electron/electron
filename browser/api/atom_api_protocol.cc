// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_protocol.h"

#include "base/memory/weak_ptr.h"
#include "browser/atom_browser_context.h"
#include "browser/net/atom_url_request_job_factory.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_job.h"
#include "net/url_request/url_request_simple_job.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

namespace atom {

namespace api {

typedef net::URLRequestJobFactory::ProtocolHandler ProtocolHandler;

namespace {

// The protocol module object.
v8::Persistent<v8::Object> g_protocol_object;

// Registered protocol handlers.
typedef std::map<std::string, v8::Persistent<v8::Function>> HandlersMap;
static HandlersMap g_handlers;

// Emit an event for the protocol module.
void EmitEventInUI(const std::string& event, const std::string& parameter) {
  v8::HandleScope scope;

  v8::Local<v8::Value> argv[] = {
    v8::String::New(event.data(), event.size()),
    v8::String::New(parameter.data(), parameter.size()),
  };
  node::MakeCallback(g_protocol_object, "emit", arraysize(argv), argv);
}

// Convert the URLRequest object to V8 object.
v8::Handle<v8::Object> ConvertURLRequestToV8Object(
    const net::URLRequest* request) {
  v8::Local<v8::Object> obj = v8::Object::New();
  obj->Set(v8::String::New("method"),
           v8::String::New(request->method().c_str()));
  obj->Set(v8::String::New("url"),
           v8::String::New(request->url().spec().c_str()));
  obj->Set(v8::String::New("referrer"),
           v8::String::New(request->referrer().c_str()));
  return obj;
}

// Get the job factory.
AtomURLRequestJobFactory* GetRequestJobFactory() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  return static_cast<AtomURLRequestJobFactory*>(
      const_cast<net::URLRequestJobFactory*>(
          static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
              GetRequestContext()->GetURLRequestContext()->job_factory()));
}

class URLRequestStringJob : public net::URLRequestSimpleJob {
 public:
  URLRequestStringJob(net::URLRequest* request,
                      net::NetworkDelegate* network_delegate,
                      const std::string& mime_type,
                      const std::string& charset,
                      const std::string& data)
      : net::URLRequestSimpleJob(request, network_delegate),
        mime_type_(mime_type),
        charset_(charset),
        data_(data) {
  }

  // URLRequestSimpleJob:
  virtual int GetData(std::string* mime_type,
                      std::string* charset,
                      std::string* data,
                      const net::CompletionCallback& callback) const OVERRIDE {
    *mime_type = mime_type_;
    *charset = charset_;
    *data = data_;
    return net::OK;
  }

 private:
  std::string mime_type_;
  std::string charset_;
  std::string data_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestStringJob);
};

// Ask JS which type of job it wants, and then delegate corresponding methods.
class AdapterRequestJob : public net::URLRequestJob {
 public:
  AdapterRequestJob(ProtocolHandler* protocol_handler,
                    net::URLRequest* request,
                    net::NetworkDelegate* network_delegate)
      : URLRequestJob(request, network_delegate),
        protocol_handler_(protocol_handler),
        weak_factory_(this) {
  }

 protected:
  // net::URLRequestJob:
  virtual void Start() OVERRIDE {
    DCHECK(!real_job_);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AdapterRequestJob::GetJobTypeInUI,
                   weak_factory_.GetWeakPtr()));
  }

  virtual void Kill() OVERRIDE {
    DCHECK(real_job_);
    real_job_->Kill();
  }

  virtual bool ReadRawData(net::IOBuffer* buf,
                           int buf_size,
                           int *bytes_read) OVERRIDE {
    DCHECK(real_job_);
    // The ReadRawData is a protected method.
    switch (type_) {
      case REQUEST_STRING_JOB:
        return static_cast<URLRequestStringJob*>(real_job_.get())->
            ReadRawData(buf, buf_size, bytes_read);
      case REQUEST_FILE_JOB:
        return static_cast<net::URLRequestFileJob*>(real_job_.get())->
            ReadRawData(buf, buf_size, bytes_read);
      default:
        return net::URLRequestJob::ReadRawData(buf, buf_size, bytes_read);
    }
  }

  virtual bool IsRedirectResponse(GURL* location,
                                  int* http_status_code) OVERRIDE {
    DCHECK(real_job_);
    return real_job_->IsRedirectResponse(location, http_status_code);
  }

  virtual net::Filter* SetupFilter() const OVERRIDE {
    DCHECK(real_job_);
    return real_job_->SetupFilter();
  }

  virtual bool GetMimeType(std::string* mime_type) const OVERRIDE {
    DCHECK(real_job_);
    return real_job_->GetMimeType(mime_type);
  }

  virtual bool GetCharset(std::string* charset) OVERRIDE {
    DCHECK(real_job_);
    return real_job_->GetCharset(charset);
  }

 private:
  enum JOB_TYPE {
    REQUEST_ERROR_JOB,
    REQUEST_STRING_JOB,
    REQUEST_FILE_JOB,
  };

  void GetJobTypeInUI() {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

    // Call the JS handler.
    v8::HandleScope scope;
    v8::Handle<v8::Value> argv[] = {
      ConvertURLRequestToV8Object(request()),
    };
    v8::Handle<v8::Value> result = g_handlers[request()->url().scheme()]->Call(
        v8::Context::GetCurrent()->Global(), arraysize(argv), argv);

    // Determine the type of the job we are going to create.
    if (result->IsString()) {
      std::string data = *v8::String::Utf8Value(result);
      content::BrowserThread::PostTask(
          content::BrowserThread::IO,
          FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                     weak_factory_.GetWeakPtr(),
                     "text/plain",
                     "UTF-8",
                     data));
      return;
    } else if (result->IsObject()) {
      v8::Handle<v8::Object> obj = result->ToObject();
      std::string name = *v8::String::Utf8Value(obj->GetConstructorName());
      if (name == "RequestStringJob") {
        std::string mime_type = *v8::String::Utf8Value(obj->Get(
            v8::String::New("mimeType")));
        std::string charset = *v8::String::Utf8Value(obj->Get(
            v8::String::New("charset")));
        std::string data = *v8::String::Utf8Value(obj->Get(
            v8::String::New("data")));

        content::BrowserThread::PostTask(
            content::BrowserThread::IO,
            FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateStringJobAndStart,
                       weak_factory_.GetWeakPtr(),
                       mime_type,
                       charset,
                       data));
        return;
      } else if (name == "RequestFileJob") {
        base::FilePath path = base::FilePath::FromUTF8Unsafe(
            *v8::String::Utf8Value(obj->Get(v8::String::New("path"))));

        content::BrowserThread::PostTask(
            content::BrowserThread::IO,
            FROM_HERE,
            base::Bind(&AdapterRequestJob::CreateFileJobAndStart,
                       weak_factory_.GetWeakPtr(),
                       path));
        return;
      }
    }

    // Try the default protocol handler if we have.
    if (protocol_handler_)
      content::BrowserThread::PostTask(
          content::BrowserThread::IO,
          FROM_HERE,
          base::Bind(&AdapterRequestJob::CreateJobFromProtocolHandlerAndStart,
                     weak_factory_.GetWeakPtr(),
                     protocol_handler_));

    // Fallback to the not implemented error.
    content::BrowserThread::PostTask(
        content::BrowserThread::IO,
        FROM_HERE,
        base::Bind(&AdapterRequestJob::CreateErrorJobAndStart,
                   weak_factory_.GetWeakPtr(),
                   net::ERR_NOT_IMPLEMENTED));
  }

  void CreateErrorJobAndStart(int error_code) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

    type_ = REQUEST_ERROR_JOB;
    real_job_ = new net::URLRequestErrorJob(
        request(), network_delegate(), error_code);
    real_job_->Start();
  }

  void CreateStringJobAndStart(const std::string& mime_type,
                               const std::string& charset,
                               const std::string& data) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

    type_ = REQUEST_STRING_JOB;
    real_job_ = new URLRequestStringJob(
        request(), network_delegate(), mime_type, charset, data);
    real_job_->Start();
  }

  void CreateFileJobAndStart(const base::FilePath& path) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

    type_ = REQUEST_FILE_JOB;
    real_job_ = new net::URLRequestFileJob(request(), network_delegate(), path);
    real_job_->Start();
  }

  void CreateJobFromProtocolHandlerAndStart(ProtocolHandler* protocol_handler) {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
    DCHECK(protocol_handler_);
    real_job_ = protocol_handler->MaybeCreateJob(request(),
                                                 network_delegate());
    if (!real_job_)
      CreateErrorJobAndStart(net::ERR_NOT_IMPLEMENTED);
    else
      real_job_->Start();
  }

  scoped_refptr<net::URLRequestJob> real_job_;

  // Type of the delegated url request job.
  JOB_TYPE type_;

  // Default protocol handler.
  ProtocolHandler* protocol_handler_;

  base::WeakPtrFactory<AdapterRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AdapterRequestJob);
};

// Always return the same AdapterRequestJob for all requests, because the
// content API needs the ProtocolHandler to return a job immediately, and
// getting the real job from the JS requires asynchronous calls, so we have
// to create an adapter job first.
// Users can also pass an extra ProtocolHandler as the fallback one when
// registered handler doesn't want to deal with the request.
class AdapterProtocolHandler : public ProtocolHandler {
 public:
  AdapterProtocolHandler(ProtocolHandler* protocol_handler = NULL)
      : protocol_handler_(protocol_handler) {
  }

  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE {
    return new AdapterRequestJob(protocol_handler_.get(),
                                 request,
                                 network_delegate);
  }

 private:
  scoped_ptr<ProtocolHandler> protocol_handler_;

  DISALLOW_COPY_AND_ASSIGN(AdapterProtocolHandler);
};

}  // namespace

// static
v8::Handle<v8::Value> Protocol::RegisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (g_handlers.find(scheme) != g_handlers.end() ||
      net::URLRequest::IsHandledProtocol(scheme))
    return node::ThrowError("The scheme is already registered");

  // Store the handler in a map.
  if (!args[1]->IsFunction())
    return node::ThrowError("Handler must be a function");
  g_handlers[scheme] = v8::Persistent<v8::Function>::New(
      node::node_isolate, v8::Handle<v8::Function>::Cast(args[1]));

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&RegisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::UnregisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));

  // Erase the handler from map.
  HandlersMap::iterator it(g_handlers.find(scheme));
  if (it == g_handlers.end())
    return node::ThrowError("The scheme has not been registered");
  g_handlers.erase(it);

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&UnregisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::IsHandledProtocol(const v8::Arguments& args) {
  return v8::Boolean::New(net::URLRequest::IsHandledProtocol(
      *v8::String::Utf8Value(args[0])));
}

// static
v8::Handle<v8::Value> Protocol::InterceptProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (scheme == "https" || scheme == "http")
    return node::ThrowError("Intercepting http protocol is not supported.");

  content::BrowserThread::PostTask(content::BrowserThread::IO,
                                   FROM_HERE,
                                   base::Bind(&UnregisterProtocolInIO, scheme));

  return v8::Undefined();
}

// static
v8::Handle<v8::Value> Protocol::UninterceptProtocol(const v8::Arguments& args) {
  return v8::Undefined();
}

// static
void Protocol::RegisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, new AdapterProtocolHandler);

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "registered",
                                              scheme));
}

// static
void Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  AtomURLRequestJobFactory* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, NULL);

  content::BrowserThread::PostTask(content::BrowserThread::UI,
                                   FROM_HERE,
                                   base::Bind(&EmitEventInUI,
                                              "unregistered",
                                              scheme));
}

// static
void Protocol::InterceptProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
}

// static
void Protocol::UninterceptProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
}

// static
void Protocol::Initialize(v8::Handle<v8::Object> target) {
  // Remember the protocol object, used for emitting event later.
  g_protocol_object = v8::Persistent<v8::Object>::New(
      node::node_isolate, target);

  node::SetMethod(target, "registerProtocol", RegisterProtocol);
  node::SetMethod(target, "unregisterProtocol", UnregisterProtocol);
  node::SetMethod(target, "isHandledProtocol", IsHandledProtocol);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_protocol, atom::api::Protocol::Initialize)
