// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_IO_THREAD_H_
#define BRIGHTRAY_BROWSER_IO_THREAD_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/browser_thread_delegate.h"

namespace net {
class URLRequestContext;
}

namespace brightray {

class IOThread : public content::BrowserThreadDelegate {
 public:
  IOThread();
  ~IOThread() override;

 protected:
  // BrowserThreadDelegate Implementation, runs on the IO thread.
  void Init() override;
  void CleanUp() override;

 private:
  std::unique_ptr<net::URLRequestContext> url_request_context_;

  DISALLOW_COPY_AND_ASSIGN(IOThread);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_IO_THREAD_H_
