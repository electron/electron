// Copyright (c) 2024 Switchboard
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/crypto_module_delegate_nss.h"

#include "crypto/nss_crypto_module_delegate.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_helper/callback.h"
#include "shell/common/v8_value_serializer.h"

std::string GetPasswordFromArgs(gin::Arguments* args) {
  if (args->Length() == 2) {
    return "";
  }

  v8::Local<v8::Value> val;
  args->GetNext(&val);
  if (val->IsNull() || val->IsUndefined()) {
    return "";
  }

  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::String> strVal;
  if (!val->ToString(context).ToLocal(&strVal)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "could not convert value to string");
    return "";
  }

  std::string password;
  if (!gin::ConvertFromV8(isolate, strVal, &password)) {
    gin_helper::ErrorThrower(isolate).ThrowError(
        "could not convert value to string");
    return "";
  }

  return password;
}

ChromeNSSCryptoModuleDelegate::ChromeNSSCryptoModuleDelegate(
    const net::HostPortPair& server)
    : server_(server),
      event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
             base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ChromeNSSCryptoModuleDelegate::~ChromeNSSCryptoModuleDelegate() = default;

std::string ChromeNSSCryptoModuleDelegate::RequestPassword(
    const std::string& token_name,
    bool retry,
    bool* cancelled) {
  DCHECK(!event_.IsSignaled());
  event_.Reset();

  if (content::GetUIThreadTaskRunner({})->PostTask(
          FROM_HERE,
          base::BindOnce(
              &ChromeNSSCryptoModuleDelegate::RequestPasswordOnUIThread, this,
              token_name, retry))) {
    base::ScopedAllowBaseSyncPrimitives allow_wait;
    event_.Wait();
  }
  *cancelled = cancelled_;
  return password_;
}

void ChromeNSSCryptoModuleDelegate::RequestPasswordOnUIThread(
    const std::string& token_name,
    bool retry) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  bool prevent_default = electron::api::App::Get()->Emit(
      "client-certificate-request-password", server_.host(), token_name, retry,
      base::BindOnce(&ChromeNSSCryptoModuleDelegate::OnPassword,
                     // FIXME: if the event is not handled, the delegate
                     // will be leaked. But using weakptr would have the
                     // following drawkback: if the user calls callback
                     // without calling preventDefault, electron will
                     // will trigger a debug assertion when trying to reference
                     // the weak pointer sequence_checker.cc(21)] Check failed:
                     // checker.CalledOnValidSequence(&bound_at)
                     this));
  if (!prevent_default && !event_.IsSignaled()) {
    password_ = "";
    cancelled_ = true;
    event_.Signal();
  }
}

void ChromeNSSCryptoModuleDelegate::OnPassword(gin::Arguments* args) {
  password_ = GetPasswordFromArgs(args);
  cancelled_ = password_.empty();
  event_.Signal();
}
