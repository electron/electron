// Copyright (c) 2018 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_
#define ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_

#include <string>
#include <vector>

#include "base/callback.h"

namespace in_app_purchase {

// --------------------------- Structures ---------------------------

struct ProductSubscriptionPeriod {
  int numberOfUnits;
  int unit;

  ProductSubscriptionPeriod(const ProductSubscriptionPeriod&);
  ProductSubscriptionPeriod();
  ~ProductSubscriptionPeriod();
};

struct ProductDiscount {
  std::string identifier;
  int type;
  double price = 0.0;
  std::string priceLocale;
  int paymentMode;
  int numberOfPeriods;
  ProductSubscriptionPeriod subscriptionPeriod;

  ProductDiscount(const ProductDiscount&);
  ProductDiscount();
  ~ProductDiscount();
};

struct Product {
  // Product Identifier
  std::string productIdentifier;

  // Product Attributes
  std::string localizedDescription;
  std::string localizedTitle;
  std::string contentVersion;
  std::vector<uint32_t> contentLengths;

  // Pricing Information
  double price = 0.0;
  std::string formattedPrice;
  std::string currencyCode;
  absl::optional<ProductDiscount> introductoryPrice;
  std::vector<ProductDiscount> discounts;
  std::string subscriptionGroupIdentifier;
  absl::optional<ProductSubscriptionPeriod> subscriptionPeriod;

  // Downloadable Content Information
  bool isDownloadable = false;
  std::string downloadContentVersion;
  std::vector<uint32_t> downloadContentLengths;

  Product(const Product&);
  Product();
  ~Product();
};

// --------------------------- Typedefs ---------------------------

typedef base::OnceCallback<void(std::vector<in_app_purchase::Product>)>
    InAppPurchaseProductsCallback;

// --------------------------- Functions ---------------------------

void GetProducts(const std::vector<std::string>& productIDs,
                 InAppPurchaseProductsCallback callback);

}  // namespace in_app_purchase

#endif  // ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_
