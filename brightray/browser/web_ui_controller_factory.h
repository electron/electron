// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_WEB_UI_CONTROLLER_FACTORY_H_
#define BRIGHTRAY_BROWSER_WEB_UI_CONTROLLER_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace brightray {

class BrowserContext;

class WebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  static WebUIControllerFactory* GetInstance();

  WebUIControllerFactory();
  ~WebUIControllerFactory() override;

  content::WebUI::TypeID GetWebUIType(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  bool UseWebUIForURL(content::BrowserContext* browser_context,
                      const GURL& url) const override;
  bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;
  std::unique_ptr<content::WebUIController> CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

 private:
  friend struct base::DefaultSingletonTraits<WebUIControllerFactory>;

  DISALLOW_COPY_AND_ASSIGN(WebUIControllerFactory);
};

}  // namespace brightray

#endif  // BRIGHTRAY_BROWSER_WEB_UI_CONTROLLER_FACTORY_H_
