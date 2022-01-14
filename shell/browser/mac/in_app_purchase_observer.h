// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
#define ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_

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

struct PaymentDiscount {
  std::string identifier;
  std::string keyIdentifier;
  std::string nonce;
  std::string signature;
  int timestamp;

  PaymentDiscount();
  PaymentDiscount(const PaymentDiscount&);
  ~PaymentDiscount();
};

struct Payment {
  std::string productIdentifier = "";
  int quantity = 1;
  std::string applicationUsername;
  PaymentDiscount paymentDiscount;

  Payment();
  Payment(const Payment&);
  ~Payment();
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

  // disable copy
  TransactionObserver(const TransactionObserver&) = delete;
  TransactionObserver& operator=(const TransactionObserver&) = delete;

  virtual void OnTransactionsUpdated(
      const std::vector<Transaction>& transactions) = 0;

 private:
  InAppTransactionObserver* observer_;

  base::WeakPtrFactory<TransactionObserver> weak_ptr_factory_{this};
};

}  // namespace in_app_purchase

#endif  // ELECTRON_SHELL_BROWSER_MAC_IN_APP_PURCHASE_OBSERVER_H_
