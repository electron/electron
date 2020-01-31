// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_
#define SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace electron {

class ElectronWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static ElectronWebUIControllerFactory* GetInstance();

  ElectronWebUIControllerFactory();
  ~ElectronWebUIControllerFactory() override;

  // content::WebUIControllerFactory:
  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) override;

 private:
  friend struct base::DefaultSingletonTraits<ElectronWebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(ElectronWebUIControllerFactory);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_WEB_UI_CONTROLLER_FACTORY_H_
