// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_CONTENT_CLIENT_H_
#define ATOM_APP_ATOM_CONTENT_CLIENT_H_

#include <string>
#include <vector>

#include "brightray/common/content_client.h"

namespace atom {

class AtomContentClient : public brightray::ContentClient {
 public:
  AtomContentClient();
  virtual ~AtomContentClient();

 protected:
  // content::ContentClient:
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  void AddAdditionalSchemes(
      std::vector<std::string>* standard_schemes,
      std::vector<std::string>* savable_schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomContentClient);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_CONTENT_CLIENT_H_
