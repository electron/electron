// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_APP_ELECTRON_CONTENT_CLIENT_H_
#define ELECTRON_APP_ELECTRON_CONTENT_CLIENT_H_

#include <set>
#include <string>
#include <vector>

#include "brightray/common/content_client.h"

namespace electron {

class ElectronContentClient : public brightray::ContentClient {
 public:
  ElectronContentClient();
  virtual ~ElectronContentClient();

 protected:
  // content::ContentClient:
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  void AddAdditionalSchemes(
      std::vector<url::SchemeWithType>* standard_schemes,
      std::vector<std::string>* savable_schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
  void AddServiceWorkerSchemes(
      std::set<std::string>* service_worker_schemes) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ElectronContentClient);
};

}  // namespace electron

#endif  // ELECTRON_APP_ELECTRON_CONTENT_CLIENT_H_
