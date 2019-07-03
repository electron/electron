// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ATOM_WEB_UI_CONTROLLER_FACTORY_H_
#define SHELL_BROWSER_ATOM_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace electron {

class AtomWebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static AtomWebUIControllerFactory* GetInstance();

  AtomWebUIControllerFactory();
  ~AtomWebUIControllerFactory() override;

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
  friend struct base::DefaultSingletonTraits<AtomWebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(AtomWebUIControllerFactory);
};

}  // namespace electron

#endif  // SHELL_BROWSER_ATOM_WEB_UI_CONTROLLER_FACTORY_H_
