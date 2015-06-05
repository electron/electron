// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

namespace atom {

CommonWebContentsDelegate::CommonWebContentsDelegate(bool is_guest)
    : is_guest_(is_guest) {
}

CommonWebContentsDelegate::~CommonWebContentsDelegate() {
}

}  // namespace atom
