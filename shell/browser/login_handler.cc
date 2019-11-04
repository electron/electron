// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/login_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "shell/browser/api/atom_api_web_contents.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"

using content::BrowserThread;

namespace electron {

LoginHandler::LoginHandler(const net::AuthChallengeInfo& auth_info,
                           content::WebContents* web_contents,
                           LoginAuthRequiredCallback auth_required_callback)
    : WebContentsObserver(web_contents),
      auth_info_(auth_info),
      auth_required_callback_(std::move(auth_required_callback)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTask(
      FROM_HERE, {base::CurrentThread()},
      base::BindOnce(&LoginHandler::EmitEvent, weak_factory_.GetWeakPtr()));
}

void LoginHandler::EmitEvent() {
  auto api_web_contents =
      api::WebContents::From(v8::Isolate::GetCurrent(), web_contents());
  if (api_web_contents.IsEmpty()) {
    std::move(auth_required_callback_).Run(base::nullopt);
    return;
  }

  base::DictionaryValue request_details;
  bool default_prevented =
      api_web_contents->Emit("login", std::move(request_details), auth_info_,
                             base::BindOnce(&LoginHandler::CallbackFromJS,
                                            weak_factory_.GetWeakPtr()));
  if (!default_prevented) {
    std::move(auth_required_callback_).Run(base::nullopt);
  }
}

LoginHandler::~LoginHandler() = default;

void LoginHandler::CallbackFromJS(base::string16 username,
                                  base::string16 password) {
  std::move(auth_required_callback_)
      .Run(net::AuthCredentials(username, password));
}

}  // namespace electron
