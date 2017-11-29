// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/atom_api_in_app_purchase.h"
#include "atom/common/native_mate_converters/callback.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace mate {

v8::Local<v8::Value> Converter<in_app_purchase::Payment>::ToV8(
    v8::Isolate* isolate,
    const in_app_purchase::Payment& payment) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("productIdentifier", payment.productIdentifier);
  dict.Set("quantity", payment.quantity);
  return dict.GetHandle();
}

v8::Local<v8::Value> Converter<in_app_purchase::Transaction>::ToV8(
    v8::Isolate* isolate,
    const in_app_purchase::Transaction& transaction) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.SetHidden("simple", true);
  dict.Set("transactionIdentifier", transaction.transactionIdentifier);
  dict.Set("transactionDate", transaction.transactionDate);
  dict.Set("originalTransactionIdentifier",
           transaction.originalTransactionIdentifier);
  dict.Set("transactionState", transaction.transactionState);

  dict.Set("errorCode", transaction.errorCode);
  dict.Set("errorMessage", transaction.errorMessage);

  return dict.GetHandle();
}
}  // namespace mate

namespace {

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
#if defined(OS_MACOSX)
  dict.SetMethod("canMakePayments", &in_app_purchase::CanMakePayments);
  dict.SetMethod("getReceiptURL", &in_app_purchase::GetReceiptURL);
  dict.SetMethod("purchaseProduct", &in_app_purchase::PurchaseProduct);
  dict.SetMethod("addTransactionListener",
                 &in_app_purchase::AddTransactionObserver);
#endif
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_in_app_purchase, Initialize)
