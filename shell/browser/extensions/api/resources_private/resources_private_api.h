// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_

#include "base/macros.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

class ResourcesPrivateGetStringsFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("resourcesPrivate.getStrings",
                             RESOURCESPRIVATE_GETSTRINGS)
  ResourcesPrivateGetStringsFunction();

 protected:
  ~ResourcesPrivateGetStringsFunction() override;

  // Override from ExtensionFunction:
  ExtensionFunction::ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourcesPrivateGetStringsFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_RESOURCES_PRIVATE_RESOURCES_PRIVATE_API_H_
