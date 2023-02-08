// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_H_
#define ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_H_

#include <string>

#include "base/functional/callback.h"

namespace in_app_purchase {

// --------------------------- Typedefs ---------------------------

typedef base::OnceCallback<void(bool isProductValid)> InAppPurchaseCallback;

// --------------------------- Functions ---------------------------

bool CanMakePayments();

void RestoreCompletedTransactions();

void FinishAllTransactions();

void FinishTransactionByDate(const std::string& date);

std::string GetReceiptURL();

void PurchaseProduct(const std::string& productID,
                     int quantity,
                     const std::string& username,
                     InAppPurchaseCallback callback);

}  // namespace in_app_purchase

#endif  // ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_H_
