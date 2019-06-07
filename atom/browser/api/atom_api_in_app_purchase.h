// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
#define ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_

#include <string>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/mac/in_app_purchase.h"
#include "atom/browser/mac/in_app_purchase_observer.h"
#include "atom/browser/mac/in_app_purchase_product.h"
#include "atom/common/promise_util.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class InAppPurchase : public mate::EventEmitter<InAppPurchase>,
                      public in_app_purchase::TransactionObserver {
 public:
  static mate::Handle<InAppPurchase> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit InAppPurchase(v8::Isolate* isolate);
  ~InAppPurchase() override;

  v8::Local<v8::Promise> PurchaseProduct(const std::string& product_id,
                                         mate::Arguments* args);

  v8::Local<v8::Promise> GetProducts(const std::vector<std::string>& productIDs,
                                     mate::Arguments* args);

  // TransactionObserver:
  void OnTransactionsUpdated(
      const std::vector<in_app_purchase::Transaction>& transactions) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InAppPurchase);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
