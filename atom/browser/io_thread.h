// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_IO_THREAD_H_
#define ATOM_BROWSER_IO_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "content/public/browser/browser_thread_delegate.h"

namespace net {
class URLRequestContext;
class URLRequestContextGetter;
}  // namespace net

namespace net_log {
class ChromeNetLog;
}

namespace atom {

class IOThread : public content::BrowserThreadDelegate {
 public:
  explicit IOThread(net_log::ChromeNetLog* net_log);
  ~IOThread() override;

  net::URLRequestContextGetter* GetRequestContext() {
    return url_request_context_getter_.get();
  }

 protected:
  // BrowserThreadDelegate Implementation, runs on the IO thread.
  void Init() override;
  void CleanUp() override;

 private:
  // The NetLog is owned by the browser process, to allow logging from other
  // threads during shutdown, but is used most frequently on the IOThread.
  net_log::ChromeNetLog* net_log_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(IOThread);
};

}  // namespace atom

#endif  // ATOM_BROWSER_IO_THREAD_H_
