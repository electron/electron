// Copyright (c) 2017 Amaplex Software, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_

#include <string>
#include <vector>

#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/mac/in_app_purchase_observer.h"
#include "v8/include/v8-forward.h"

namespace electron::api {

class InAppPurchase final : public gin::Wrappable<InAppPurchase>,
                            public gin_helper::EventEmitterMixin<InAppPurchase>,
                            private in_app_purchase::TransactionObserver {
 public:
  static InAppPurchase* Create(v8::Isolate* isolate);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  static const char* GetClassName() { return "InAppPurchase"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  void Trace(cppgc::Visitor* visitor) const override;

  // disable copy
  InAppPurchase(const InAppPurchase&) = delete;
  InAppPurchase& operator=(const InAppPurchase&) = delete;

  // Make public for cppgc::MakeGarbageCollected.
  InAppPurchase();
  ~InAppPurchase() override;

 private:
  v8::Local<v8::Promise> PurchaseProduct(const std::string& product_id,
                                         gin::Arguments* args);

  v8::Local<v8::Promise> GetProducts(const std::vector<std::string>& productIDs,
                                     gin::Arguments* args);

  void OnTransactionsUpdated(
      const std::vector<in_app_purchase::Transaction>& transactions);

  gin::WeakCellFactory<InAppPurchase> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_IN_APP_PURCHASE_H_
