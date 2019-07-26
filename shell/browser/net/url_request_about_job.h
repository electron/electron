// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_NET_URL_REQUEST_ABOUT_JOB_H_
#define SHELL_BROWSER_NET_URL_REQUEST_ABOUT_JOB_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_job.h"

namespace electron {

class URLRequestAboutJob : public net::URLRequestJob {
 public:
  URLRequestAboutJob(net::URLRequest*, net::NetworkDelegate*);

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  bool GetMimeType(std::string* mime_type) const override;

 private:
  ~URLRequestAboutJob() override;
  void StartAsync();

  base::WeakPtrFactory<URLRequestAboutJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestAboutJob);
};

}  // namespace electron

#endif  // SHELL_BROWSER_NET_URL_REQUEST_ABOUT_JOB_H_
