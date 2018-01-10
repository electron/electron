// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MAC_IN_APP_PURCHASE_H_
#define ATOM_BROWSER_MAC_IN_APP_PURCHASE_H_

#include <string>

#include "base/callback.h"

namespace in_app_purchase {

// --------------------------- Typedefs ---------------------------

typedef base::Callback<void(bool isProductValid)> InAppPurchaseCallback;

// --------------------------- Functions ---------------------------

bool CanMakePayments(void);

std::string GetReceiptURL(void);

void PurchaseProduct(const std::string& productID,
                     int quantity,
                     const InAppPurchaseCallback& callback);

}  // namespace in_app_purchase

#endif  // ATOM_BROWSER_MAC_IN_APP_PURCHASE_H_
