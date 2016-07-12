// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define BRAVE_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include <string>
#include <vector>
#include "atom/browser/atom_browser_context.h"

class PrefChangeRegistrar;

namespace syncable_prefs {
class PrefServiceSyncable;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

class BravePermissionManager;

class BraveBrowserContext : public atom::AtomBrowserContext {
 public:
  BraveBrowserContext(const std::string& partition, bool in_memory);
  ~BraveBrowserContext() override;

  static BraveBrowserContext*
      FromBrowserContext(content::BrowserContext* browser_context);

  // brightray::URLRequestContextGetter::Delegate:
  std::unique_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* protocol_handlers) override;

  // brightray::BrowserContext:
  void RegisterPrefs(PrefRegistrySimple* pref_registry) override;

  // content::BrowserContext:
  net::NetworkDelegate* CreateNetworkDelegate() override;
  content::PermissionManager* GetPermissionManager() override;
  content::ResourceContext* GetResourceContext() override;

  // atom::AtomBrowserContext
  atom::AtomNetworkDelegate* network_delegate() const override {
      return network_delegate_; }

  BraveBrowserContext* original_context();
  BraveBrowserContext* otr_context();

  user_prefs::PrefRegistrySyncable* pref_registry() const {
    return pref_registry_.get(); }

  syncable_prefs::PrefServiceSyncable* user_prefs() const {
    return user_prefs_.get(); }

  PrefChangeRegistrar* user_prefs_change_registrar() const {
    return user_prefs_registrar_.get(); }

  const std::string& partition() const { return partition_; }

  void AddOverlayPref(const std::string name) {
    overlay_pref_names_.push_back(name.c_str()); }

 private:
  void OnPrefsLoaded(bool success);
  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  std::unique_ptr<syncable_prefs::PrefServiceSyncable> user_prefs_;
  std::unique_ptr<PrefChangeRegistrar> user_prefs_registrar_;
  std::vector<const char*> overlay_pref_names_;

  std::unique_ptr<BravePermissionManager> permission_manager_;

  atom::AtomNetworkDelegate* network_delegate_;

  BraveBrowserContext* original_context_;
  scoped_refptr<BraveBrowserContext> otr_context_;
  const std::string partition_;

  DISALLOW_COPY_AND_ASSIGN(BraveBrowserContext);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_ATOM_BROWSER_CONTEXT_H_
