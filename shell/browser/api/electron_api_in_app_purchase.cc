// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_in_app_purchase.h"

#include <string>
#include <utility>
#include <vector>

#include "gin/handle.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

namespace gin {

template <>
struct Converter<in_app_purchase::PaymentDiscount> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const in_app_purchase::PaymentDiscount& paymentDiscount) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("identifier", paymentDiscount.identifier);
    dict.Set("keyIdentifier", paymentDiscount.keyIdentifier);
    dict.Set("nonce", paymentDiscount.nonce);
    dict.Set("signature", paymentDiscount.signature);
    dict.Set("timestamp", paymentDiscount.timestamp);
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::Payment> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Payment& payment) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("productIdentifier", payment.productIdentifier);
    dict.Set("quantity", payment.quantity);
    dict.Set("applicationUsername", payment.applicationUsername);
    if (payment.paymentDiscount.has_value()) {
      dict.Set("paymentDiscount", payment.paymentDiscount.value());
    }
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::Transaction> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Transaction& val) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("transactionIdentifier", val.transactionIdentifier);
    dict.Set("transactionDate", val.transactionDate);
    dict.Set("originalTransactionIdentifier",
             val.originalTransactionIdentifier);
    dict.Set("transactionState", val.transactionState);
    dict.Set("errorCode", val.errorCode);
    dict.Set("errorMessage", val.errorMessage);
    dict.Set("payment", val.payment);
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::ProductSubscriptionPeriod> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const in_app_purchase::ProductSubscriptionPeriod&
          productSubscriptionPeriod) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("numberOfUnits", productSubscriptionPeriod.numberOfUnits);
    dict.Set("unit", productSubscriptionPeriod.unit);
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::ProductDiscount> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const in_app_purchase::ProductDiscount& productDiscount) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("identifier", productDiscount.identifier);
    dict.Set("type", productDiscount.type);
    dict.Set("price", productDiscount.price);
    dict.Set("priceLocale", productDiscount.priceLocale);
    dict.Set("paymentMode", productDiscount.paymentMode);
    dict.Set("numberOfPeriods", productDiscount.numberOfPeriods);
    if (productDiscount.subscriptionPeriod.has_value()) {
      dict.Set("subscriptionPeriod",
               productDiscount.subscriptionPeriod.value());
    }
    return dict.GetHandle();
  }
};

template <>
struct Converter<in_app_purchase::Product> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const in_app_purchase::Product& val) {
    auto dict = gin_helper::Dictionary::CreateEmpty(isolate);
    dict.Set("productIdentifier", val.productIdentifier);
    dict.Set("localizedDescription", val.localizedDescription);
    dict.Set("localizedTitle", val.localizedTitle);

    // Pricing Information
    dict.Set("price", val.price);
    dict.Set("formattedPrice", val.formattedPrice);
    dict.Set("currencyCode", val.currencyCode);
    if (val.introductoryPrice.has_value()) {
      dict.Set("introductoryPrice", val.introductoryPrice.value());
    }
    dict.Set("discounts", val.discounts);
    dict.Set("subscriptionGroupIdentifier", val.subscriptionGroupIdentifier);
    if (val.subscriptionPeriod.has_value()) {
      dict.Set("subscriptionPeriod", val.subscriptionPeriod.value());
    }
    // Downloadable Content Information
    dict.Set("isDownloadable", val.isDownloadable);
    dict.Set("downloadContentVersion", val.downloadContentVersion);
    dict.Set("downloadContentLengths", val.downloadContentLengths);

    return dict.GetHandle();
  }
};

}  // namespace gin

namespace electron::api {

gin::DeprecatedWrapperInfo InAppPurchase::kWrapperInfo = {
    gin::kEmbedderNativeGin};

#if BUILDFLAG(IS_MAC)
// static
gin::Handle<InAppPurchase> InAppPurchase::Create(v8::Isolate* isolate) {
  return gin::CreateHandle(isolate, new InAppPurchase());
}

gin::ObjectTemplateBuilder InAppPurchase::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<InAppPurchase>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("canMakePayments", &in_app_purchase::CanMakePayments)
      .SetMethod("restoreCompletedTransactions",
                 &in_app_purchase::RestoreCompletedTransactions)
      .SetMethod("getReceiptURL", &in_app_purchase::GetReceiptURL)
      .SetMethod("purchaseProduct", &InAppPurchase::PurchaseProduct)
      .SetMethod("finishAllTransactions",
                 &in_app_purchase::FinishAllTransactions)
      .SetMethod("finishTransactionByDate",
                 &in_app_purchase::FinishTransactionByDate)
      .SetMethod("getProducts", &InAppPurchase::GetProducts);
}

const char* InAppPurchase::GetTypeName() {
  return "InAppPurchase";
}

InAppPurchase::InAppPurchase() = default;

InAppPurchase::~InAppPurchase() = default;

v8::Local<v8::Promise> InAppPurchase::PurchaseProduct(
    const std::string& product_id,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<bool> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  int quantity = 1;
  args->GetNext(&quantity);
  std::string username = "";
  args->GetNext(&username);

  in_app_purchase::PurchaseProduct(
      product_id, quantity, username,
      base::BindOnce(gin_helper::Promise<bool>::ResolvePromise,
                     std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> InAppPurchase::GetProducts(
    const std::vector<std::string>& productIDs,
    gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  gin_helper::Promise<std::vector<in_app_purchase::Product>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  in_app_purchase::GetProducts(
      productIDs,
      base::BindOnce(gin_helper::Promise<
                         std::vector<in_app_purchase::Product>>::ResolvePromise,
                     std::move(promise)));

  return handle;
}

void InAppPurchase::OnTransactionsUpdated(
    const std::vector<in_app_purchase::Transaction>& transactions) {
  Emit("transactions-updated", transactions);
}
#endif

}  // namespace electron::api

namespace {

using electron::api::InAppPurchase;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
#if BUILDFLAG(IS_MAC)
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("inAppPurchase", InAppPurchase::Create(isolate));
#endif
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_in_app_purchase, Initialize)
