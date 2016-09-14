// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_COMPONENT_UPDATER_ATOM_UPDATE_CLIENT_CONFIG_H_
#define ATOM_BROWSER_COMPONENT_UPDATER_ATOM_UPDATE_CLIENT_CONFIG_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/component_updater/configurator_impl.h"
#include "extensions/browser/updater/update_client_config.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class AtomUpdateClientConfig : public UpdateClientConfig {
 public:
  explicit AtomUpdateClientConfig(content::BrowserContext* context);

  int InitialDelay() const override;
  int NextCheckDelay() const override;
  int StepDelay() const override;
  int OnDemandDelay() const override;
  int UpdateDelay() const override;
  std::vector<GURL> UpdateUrl() const override;
  std::vector<GURL> PingUrl() const override;
  base::Version GetBrowserVersion() const override;
  std::string GetChannel() const override;
  std::string GetBrand() const override;
  std::string GetLang() const override;
  std::string GetOSLongName() const override;
  std::string ExtraRequestParams() const override;
  std::string GetDownloadPreference() const override;
  net::URLRequestContextGetter* RequestContext() const override;
  scoped_refptr<update_client::OutOfProcessPatcher> CreateOutOfProcessPatcher()
      const override;
  bool DeltasEnabled() const override;
  bool UseBackgroundDownloader() const override;
  bool UseCupSigning() const override;
  PrefService* GetPrefService() const override;

 protected:
  friend class base::RefCountedThreadSafe<AtomUpdateClientConfig>;
  ~AtomUpdateClientConfig() override;

 private:
  component_updater::ConfiguratorImpl impl_;

  DISALLOW_COPY_AND_ASSIGN(AtomUpdateClientConfig);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_COMPONENT_UPDATER_ATOM_UPDATE_CLIENT_CONFIG_H_
