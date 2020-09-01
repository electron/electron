// Copyright (c) 2020 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_EXTENSIONS_API_I18N_I18N_API_H_
#define SHELL_BROWSER_EXTENSIONS_API_I18N_I18N_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {

class I18nGetAcceptLanguagesFunction : public ExtensionFunction {
  ~I18nGetAcceptLanguagesFunction() override {}
  ResponseAction Run() override;
  DECLARE_EXTENSION_FUNCTION("i18n.getAcceptLanguages", I18N_GETACCEPTLANGUAGES)
};

}  // namespace extensions

#endif  // SHELL_BROWSER_EXTENSIONS_API_I18N_I18N_API_H_
