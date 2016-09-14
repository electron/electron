// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_
#define ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "brave/browser/brave_browser_context.h"
#include "native_mate/handle.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_observer.h"
#include "components/component_updater/component_updater_service.h"

namespace brave {
class BraveBrowserContext;
}

namespace atom {

namespace api {

class UpdateClient : public mate::TrackableObject<UpdateClient>,
                     public content::NotificationObserver,
                     public component_updater::ServiceObserver {
 public:
  static mate::Handle<UpdateClient> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  UpdateClient(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~UpdateClient() override;
  void RegisterComponent(const std::string& extension_id);
  void Install(const std::string& extension_id);
  void CheckForUpdate(const std::string& extension_id);
  void OnCompnentRegistered(const std::string& extension_id);

  brave::BraveBrowserContext* browser_context() {
    return static_cast<brave::BraveBrowserContext*>(browser_context_);
  }

  // content::NotificationObserver implementation.
  void Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) override;

  // ServiceObserver implementation.
  void OnEvent(Events event, const std::string& id) override;

 private:
  content::BrowserContext* browser_context_;  // not owned
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(UpdateClient);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_UPDATE_CLIENT_H_
