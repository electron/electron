// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRIGHTRAY_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_FACTORY_H_
#define BRIGHTRAY_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_FACTORY_H_

#include "base/macros.h"

namespace net {
class URLRequestContext;
}  // namespace net

namespace brightray {

class URLRequestContextGetterFactory {
 public:
  URLRequestContextGetterFactory() {}
  virtual ~URLRequestContextGetterFactory() {}

  virtual net::URLRequestContext* Create() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetterFactory);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_NET_URL_REQUEST_CONTEXT_GETTER_FACTORY_H_
