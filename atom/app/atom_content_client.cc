// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/app/atom_content_client.h"

#include <string>
#include <vector>

namespace atom {

AtomContentClient::AtomContentClient() {
}

AtomContentClient::~AtomContentClient() {
}

void AtomContentClient::AddAdditionalSchemes(
    std::vector<std::string>* standard_schemes,
    std::vector<std::string>* savable_schemes) {
  standard_schemes->push_back("chrome-extension");
}

}  // namespace atom
