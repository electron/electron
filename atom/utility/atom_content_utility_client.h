// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
#define ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_

#include <vector>

#include "base/compiler_specific.h"
#include "base/memory/scoped_vector.h"
#include "content/public/utility/content_utility_client.h"

class UtilityMessageHandler;

namespace atom {

class AtomContentUtilityClient : public content::ContentUtilityClient {
 public:
  AtomContentUtilityClient();
  ~AtomContentUtilityClient() override;

 private:
#if defined(OS_WIN)
  typedef ScopedVector<UtilityMessageHandler> Handlers;
  Handlers handlers_;
#endif

  DISALLOW_COPY_AND_ASSIGN(AtomContentUtilityClient);
};

}  // namespace atom

#endif  // ATOM_UTILITY_ATOM_CONTENT_UTILITY_CLIENT_H_
