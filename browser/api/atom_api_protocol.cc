// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_protocol.h"

#include "browser/atom_browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_error_job.h"
#include "net/url_request/url_request_file_job.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "net/url_request/url_request_simple_job.h"
#include "vendor/node/src/node.h"

namespace atom {

namespace api {

namespace {

typedef std::map<std::string, v8::Persistent<v8::Function>> HandlersMap;
static HandlersMap g_handlers;

net::URLRequestJobFactoryImpl* GetRequestJobFactory() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  // Get the job factory.
  net::URLRequestJobFactoryImpl* job_factory =
      static_cast<net::URLRequestJobFactoryImpl*>(
          const_cast<net::URLRequestJobFactory*>(
              static_cast<content::BrowserContext*>(AtomBrowserContext::Get())->
                  GetRequestContext()->GetURLRequestContext()->job_factory()));
  return job_factory;
}

// Ask JS which type of job it wants, and then delegate corresponding methods.
class AdapterRequestJob : public net::URLRequestJob {
 public:
  AdapterRequestJob(net::URLRequest* request,
                    net::NetworkDelegate* network_delegate)
      : URLRequestJob(request, network_delegate) {}

 protected:
  virtual void Start() OVERRIDE {
    DCHECK(!real_job_);
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&AdapterRequestJob::GetJobTypeInUI, base::Unretained(this)));
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
      case FILE_JOB:
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
    ERROR_JOB,
    STRING_JOB,
    FILE_JOB,
  };

  void GetJobTypeInUI() {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

    // Call the JS handler.
    v8::HandleScope scope;
    v8::Handle<v8::Value> argv[] = {
      v8::String::New(request()->url().spec().c_str()),
      v8::String::New(request()->referrer().c_str()),
    };
    v8::Handle<v8::Value> result = g_handlers[request()->url().scheme()]->Call(
        v8::Context::GetCurrent()->Global(), arraysize(argv), argv);

    // Determine the type of the job we are going to create.
    if (result->IsString()) {
      type_ = STRING_JOB;
      string_ = *v8::String::Utf8Value(result);
    } else {
      type_ = ERROR_JOB;
      error_code_ = ERR_NOT_IMPLEMENTED;
    }

    // Go back to the IO thread.
    content::BrowserThread::PostTask(
        content::BrowserThread::IO,
        FROM_HERE,
        base::Bind(&AdapterRequestJob::CreateJobAndStart,
                   base::Unretained(this)));
  }

  void CreateJobAndStart() {
    DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
    real_job_ = new net::URLRequestErrorJob(
        request(), network_delegate(), net::ERR_NOT_IMPLEMENTED);
    real_job_->Start();
  }

  scoped_refptr<net::URLRequestJob> real_job_;

  JOB_TYPE type_;
  int error_code_;
  std::string string_;
  base::FilePath file_path_;

  DISALLOW_COPY_AND_ASSIGN(AdapterRequestJob);
};

// Always return the same AdapterRequestJob for all requests, because the
// content API needs the ProtocolHandler to return a job immediately, and
// getting the real job from the JS requires asynchronous calls, so we have
// to create an adapter job first.
class AdapterProtocolHandler
    : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  AdapterProtocolHandler() {}
  virtual net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const OVERRIDE {
    return new AdapterRequestJob(request, network_delegate);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AdapterProtocolHandler);
};

}  // namespace

// static
v8::Handle<v8::Value> Protocol::RegisterProtocol(const v8::Arguments& args) {
  std::string scheme(*v8::String::Utf8Value(args[0]));
  if (g_handlers.find(scheme) != g_handlers.end())
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
void Protocol::RegisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  net::URLRequestJobFactoryImpl* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, new AdapterProtocolHandler);
}

// static
void Protocol::UnregisterProtocolInIO(const std::string& scheme) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  net::URLRequestJobFactoryImpl* job_factory(GetRequestJobFactory());
  job_factory->SetProtocolHandler(scheme, NULL);
}

// static
void Protocol::Initialize(v8::Handle<v8::Object> target) {
  node::SetMethod(target, "registerProtocol", RegisterProtocol);
  node::SetMethod(target, "unregisterProtocol", UnregisterProtocol);
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_protocol, atom::api::Protocol::Initialize)
