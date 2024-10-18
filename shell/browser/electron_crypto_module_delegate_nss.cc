// Copyright (c) 2024 Switchboard
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_crypto_module_delegate_nss.h"

#include "content/public/browser/browser_thread.h"
#include "crypto/nss_crypto_module_delegate.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/callback.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/v8_util.h"

ElectronNSSCryptoModuleDelegate::ElectronNSSCryptoModuleDelegate(
    const net::HostPortPair& server)
    : server_(server),
      event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
             base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ElectronNSSCryptoModuleDelegate::~ElectronNSSCryptoModuleDelegate() = default;

std::string ElectronNSSCryptoModuleDelegate::RequestPassword(
    const std::string& token_name,
    bool retry,
    bool* cancelled) {
  DCHECK(!event_.IsSignaled());
  event_.Reset();

  if (content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce(
              &ElectronNSSCryptoModuleDelegate::RequestPasswordOnUIThread, this,
              token_name, retry))) {
    base::ScopedAllowBaseSyncPrimitivesForTesting allow_wait;
    event_.Wait();
  }
  *cancelled = cancelled_;
  return password_;
}

void ElectronNSSCryptoModuleDelegate::RequestPasswordOnUIThread(
    const std::string& token_name,
    bool retry) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  gin::Handle<gin_helper::internal::Event> event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object = event.ToV8().As<v8::Object>();
  gin_helper::Dictionary dict(isolate, event_object);
  dict.Set("hostname", server_.host());
  dict.Set("tokenName", token_name);
  dict.Set("isRetry", retry);

  electron::api::App::Get()->EmitWithoutEvent(
      "-client-certificate-request-password", event_object,
      base::BindOnce(&ElectronNSSCryptoModuleDelegate::OnPassword, this));

  if (!event->GetDefaultPrevented()) {
    password_ = "";
    cancelled_ = true;
    event_.Signal();
  }
}

void ElectronNSSCryptoModuleDelegate::OnPassword(gin::Arguments* args) {
  args->GetNext(&password_);
  cancelled_ = password_.empty();
  event_.Signal();
}
