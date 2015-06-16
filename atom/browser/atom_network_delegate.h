// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_ATOM_NETWORK_DELEGATE_H_

#include "brightray/browser/network_delegate.h"

namespace atom {

class NetworkDelegate : public brightray::NetworkDelegate {
 public:
  NetworkDelegate();
  ~NetworkDelegate();

  int
  OnBeforeURLRequest(net::URLRequest*,
    const net::CompletionCallback&, GURL* new_url)
  override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkDelegate);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_NETWORK_DELEGATE_H_
