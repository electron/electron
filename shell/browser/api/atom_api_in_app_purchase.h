// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
#define SHELL_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_

#include <string>
#include <vector>

#include "native_mate/handle.h"
#include "shell/browser/api/event_emitter.h"
#include "shell/browser/mac/in_app_purchase.h"
#include "shell/browser/mac/in_app_purchase_observer.h"
#include "shell/browser/mac/in_app_purchase_product.h"
#include "shell/common/promise_util.h"

namespace electron {

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

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_IN_APP_PURCHASE_H_
