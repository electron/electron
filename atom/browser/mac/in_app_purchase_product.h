// Copyright (c) 2018 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_
#define ATOM_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_

#include <string>
#include <vector>

#include "base/callback.h"

namespace in_app_purchase {

// --------------------------- Structures ---------------------------

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

  // Downloadable Content Information
  bool downloadable = false;

  Product(const Product&);
  Product();
  ~Product();
};

// --------------------------- Typedefs ---------------------------

typedef base::Callback<void(const std::vector<in_app_purchase::Product>&)>
    InAppPurchaseProductsCallback;

// --------------------------- Functions ---------------------------

void GetProducts(const std::vector<std::string>& productIDs,
                 const InAppPurchaseProductsCallback& callback);

}  // namespace in_app_purchase

#endif  // ATOM_BROWSER_MAC_IN_APP_PURCHASE_PRODUCT_H_
