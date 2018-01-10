// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
#define ATOM_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_

#include <string>

#include "base/callback.h"

namespace in_app_purchase {

// --------------------------- Structures ---------------------------

struct Payment {
  std::string productIdentifier = "";
  int quantity = 1;
};

struct Transaction {
  std::string transactionIdentifier = "";
  std::string transactionDate = "";
  std::string originalTransactionIdentifier = "";
  int errorCode = 0;
  std::string errorMessage = "";
  std::string transactionState = "";
};

// --------------------------- Typedefs ---------------------------

typedef base::RepeatingCallback<void(const Payment, const Transaction)>
    InAppTransactionCallback;

// --------------------------- Functions ---------------------------

void AddTransactionObserver(const InAppTransactionCallback& callback);

}  // namespace in_app_purchase

#endif  // ATOM_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
