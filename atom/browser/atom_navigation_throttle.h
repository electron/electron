// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_NET_ATOM_NAVIGATION_THROTTLE_H_
#define ATOM_BROWSER_NET_ATOM_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace atom {

class AtomNavigationThrottle : public content::NavigationThrottle {
 public:
  AtomNavigationThrottle(content::NavigationHandle* handle);
  ~AtomNavigationThrottle() override;

  AtomNavigationThrottle::ThrottleCheckResult WillRedirectRequest() override;

  const char* GetNameForLogging() override { return "AtomNavigationThrottle"; }

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomNavigationThrottle);
};

}  // namespace atom

#endif  // ATOM_BROWSER_NET_ATOM_NAVIGATION_THROTTLE_H_
