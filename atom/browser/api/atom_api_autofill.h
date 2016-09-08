// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
#define ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "brave/browser/brave_browser_context.h"
#include "native_mate/handle.h"

namespace autofill {
class AutofillProfile;
}

namespace base {
class DictionaryValue;
class ListValue;
}

namespace brave {
class BraveBrowserContext;
}

namespace atom {

namespace api {

class Autofill : public mate::TrackableObject<Autofill> {
 public:
  static mate::Handle<Autofill> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Autofill(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~Autofill() override;

  std::string AddProfile(const base::DictionaryValue& profile);
  autofill::AutofillProfile* GetProfile(const std::string& guid);
  bool RemoveProfile(const std::string& guid);

  std::string AddCreditCard(const base::DictionaryValue& card);
  autofill::CreditCard* GetCreditCard(const std::string& guid);
  bool RemoveCreditCard(const std::string& guid);

  brave::BraveBrowserContext* browser_context() {
    return static_cast<brave::BraveBrowserContext*>(browser_context_);
  }

 private:
  content::BrowserContext* browser_context_;  // not owned

  DISALLOW_COPY_AND_ASSIGN(Autofill);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_AUTOFILL_H_
