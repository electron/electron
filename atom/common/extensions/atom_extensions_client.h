// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_
#define ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_

#include <string>
#include "chrome/common/extensions/chrome_extensions_client.h"
#include "base/lazy_instance.h"

namespace extensions {

class AtomExtensionsClient : public ChromeExtensionsClient {
 public:
  AtomExtensionsClient();
  ~AtomExtensionsClient() override;

  const std::string GetProductName() override;
  scoped_ptr<JSONFeatureProviderSource> CreateFeatureProviderSource(
      const std::string& name) const override;
  bool IsScriptableURL(const GURL& url, std::string* error) const override;
  void RegisterAPISchemaResources(ExtensionAPI* api) const override;
  std::string GetWebstoreBaseURL() const override;
  std::string GetWebstoreUpdateURL() const override;

  // Get the LazyInstance for AtomExtensionsClient.
  static AtomExtensionsClient* GetInstance();

 private:
  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsClient);
};

}  // namespace extensions

#endif  // ATOM_COMMON_EXTENSIONS_ATOM_EXTENSIONS_CLIENT_H_
