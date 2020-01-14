// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
#define SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"

#if defined(__OBJC__)
@class InAppTransactionObserver;
#else   // __OBJC__
class InAppTransactionObserver;
#endif  // __OBJC__

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
  Payment payment;

  Transaction();
  Transaction(const Transaction&);
  ~Transaction();
};

// --------------------------- Classes ---------------------------

class TransactionObserver {
 public:
  TransactionObserver();
  virtual ~TransactionObserver();

  virtual void OnTransactionsUpdated(
      const std::vector<Transaction>& transactions) = 0;

 private:
  InAppTransactionObserver* observer_;

  base::WeakPtrFactory<TransactionObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TransactionObserver);
};

}  // namespace in_app_purchase

#endif  // SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
