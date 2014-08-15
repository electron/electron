// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_browser_context.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/net/atom_url_request_job_factory.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/url_constants.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/protocol_intercept_job_factory.h"

using content::BrowserThread;

namespace atom {

AtomBrowserContext::AtomBrowserContext()
    : job_factory_(new AtomURLRequestJobFactory) {
}

AtomBrowserContext::~AtomBrowserContext() {
}

scoped_ptr<net::URLRequestJobFactory>
AtomBrowserContext::CreateURLRequestJobFactory(
    content::ProtocolHandlerMap* handlers,
    content::ProtocolHandlerScopedVector* interceptors) {
  scoped_ptr<AtomURLRequestJobFactory> job_factory(job_factory_);

  for (content::ProtocolHandlerMap::iterator it = handlers->begin();
       it != handlers->end(); ++it)
    job_factory->SetProtocolHandler(it->first, it->second.release());
  handlers->clear();

  job_factory->SetProtocolHandler(
      content::kDataScheme, new net::DataProtocolHandler);
  job_factory->SetProtocolHandler(
      content::kFileScheme, new net::FileProtocolHandler(
          BrowserThread::GetBlockingPool()->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN)));

  // Set up interceptors in the reverse order.
  scoped_ptr<net::URLRequestJobFactory> top_job_factory =
      job_factory.PassAs<net::URLRequestJobFactory>();
  content::ProtocolHandlerScopedVector::reverse_iterator it;
  for (it = interceptors->rbegin(); it != interceptors->rend(); ++it)
    top_job_factory.reset(new net::ProtocolInterceptJobFactory(
        top_job_factory.Pass(), make_scoped_ptr(*it)));
  interceptors->weak_clear();

  return top_job_factory.Pass();
}

// static
AtomBrowserContext* AtomBrowserContext::Get() {
  return static_cast<AtomBrowserContext*>(
      AtomBrowserMainParts::Get()->browser_context());
}

}  // namespace atom
