// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/login_handler.h"

#include "net/base/auth.h"

namespace atom {

LoginHandler::LoginHandler(net::AuthChallengeInfo* auth_info,
                           net::URLRequest* request)
    : auth_info_(auth_info), request_(request), weak_factory_(this) {
}

LoginHandler::~LoginHandler() {
}

void LoginHandler::OnRequestCancelled() {
}

}  // namespace atom
