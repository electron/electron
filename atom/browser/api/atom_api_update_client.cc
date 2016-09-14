// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_update_client.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_content_browser_client.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "native_mate/dictionary.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/updater/update_service.h"
#include "extensions/browser/updater/update_service_factory.h"
#include "components/update_client/crx_update_item.h"
#include "atom/browser/extensions/atom_notification_types.h"
#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/browser/api/atom_api_extension.h"
#include "atom/browser/component_updater/default_extensions.h"

namespace atom {

namespace api {

UpdateClient::UpdateClient(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context) {
  Init(isolate);
  registrar_.Add(this,
      extensions::NOTIFICATION_EXTENSION_ENABLED,
      content::NotificationService::AllBrowserContextsAndSources());
  registrar_.Add(this,
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      content::NotificationService::AllBrowserContextsAndSources());
}

UpdateClient::~UpdateClient() {
}

void UpdateClient::Observe(
     int type, const content::NotificationSource& source,
     const content::NotificationDetails& details) {
  switch (type) {
    case extensions::NOTIFICATION_EXTENSION_ENABLED: {
      Emit("extension-enabled");
      break;
    }
    case extensions::NOTIFICATION_CRX_INSTALLER_DONE: {
      Emit("crx-installer-done");
      break;
    }
  }
}

void UpdateClient::OnEvent(Events event, const std::string& id) {
  update_client::CrxUpdateItem item;
  AtomBrowserMainParts::Get()->GetComponentDetails(id, &item);
  switch (event) {
    case Events::COMPONENT_CHECKING_FOR_UPDATES:
      Emit("component-checking-for-updates", id);
      break;
    case Events::COMPONENT_WAIT:
      Emit("component-wait", id);
      break;
    case Events::COMPONENT_UPDATE_FOUND:
      Emit("component-update-found", id);
      break;
    case Events::COMPONENT_UPDATE_READY: {
      // Uninstall for upgrades
        atom::api::Extension::GetInstance()->Remove(id);
        Emit("component-update-ready", id);
      }
      break;
    case Events::COMPONENT_UPDATED: {
        auto str = item.component.version.GetString();
        Emit("component-update-updated", id, str);
      }
      break;
    case Events::COMPONENT_NOT_UPDATED:
      Emit("component-not-updated", id);
      break;
    case Events::COMPONENT_UPDATE_DOWNLOADING:
      Emit("component-update-downloading", id);
      break;
  }
}

void UpdateClient::OnCompnentRegistered(const std::string& extension_id) {
  Emit("component-registered", extension_id);
}

void UpdateClient::RegisterComponent(const std::string& extension_id) {
  auto components = AtomBrowserMainParts::Get();
  if (extension_id == kOnePasswordId) {
    components->RegisterComponentForUpdate(kOnePasswordPublicKeyStr,
        base::Bind(&UpdateClient::OnCompnentRegistered,
          base::Unretained(this), extension_id));
  } else if (extension_id == kDashlaneId) {
    components->RegisterComponentForUpdate(kDashlanePublicKeyStr,
        base::Bind(&UpdateClient::OnCompnentRegistered,
          base::Unretained(this), extension_id));
  } else if (extension_id == kLastPassId) {
    components->RegisterComponentForUpdate(kLastPassPublicKeyStr,
        base::Bind(&UpdateClient::OnCompnentRegistered,
          base::Unretained(this), extension_id));
  } else if (extension_id == kPDFJSId) {
    components->RegisterComponentForUpdate(kPDFJSPublicKeyStr,
        base::Bind(&UpdateClient::OnCompnentRegistered,
          base::Unretained(this), extension_id));
  }
}

void UpdateClient::Install(const std::string& extension_id) {
  // The front end thinks there is no extension, so make sure that's actually
  // the case. This keeps things in sync in case session-store-1 is cleared.
  atom::api::Extension::GetInstance()->Remove(extension_id);
  CheckForUpdate(extension_id);
}

void UpdateClient::CheckForUpdate(const std::string& extension_id) {
  AtomBrowserMainParts::Get()->InstallExtension(extension_id);
}

// static
mate::Handle<UpdateClient> UpdateClient::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  return mate::CreateHandle(isolate,
      new UpdateClient(isolate, browser_context));
}

// static
void UpdateClient::BuildPrototype(v8::Isolate* isolate,
                              v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "UpdateClient"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
    .SetMethod("install", &UpdateClient::Install)
    .SetMethod("checkForUpdate", &UpdateClient::CheckForUpdate)
    .SetMethod("registerComponent", &UpdateClient::RegisterComponent);
}

}  // namespace api

}  // namespace atom
