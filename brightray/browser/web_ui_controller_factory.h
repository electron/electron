// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BRIGHTRAY_BROWSER_WEB_UI_CONTROLLER_FACTORY_H_
#define BRIGHTRAY_BROWSER_WEB_UI_CONTROLLER_FACTORY_H_

#include "base/basictypes.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller_factory.h"

namespace brightray {

class BrowserContext;

class WebUIControllerFactory : public content::WebUIControllerFactory {
 public:
  explicit WebUIControllerFactory(BrowserContext* browser_context);
  virtual ~WebUIControllerFactory();

  virtual content::WebUI::TypeID GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const override;
  virtual bool UseWebUIForURL(content::BrowserContext* browser_context,
                              const GURL& url) const override;
  virtual bool UseWebUIBindingsForURL(content::BrowserContext* browser_context,
                                      const GURL& url) const override;
  virtual content::WebUIController* CreateWebUIControllerForURL(
      content::WebUI* web_ui,
      const GURL& url) const override;

  static WebUIControllerFactory* GetInstance();

 private:
  // Weak reference to the browser context.
  BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(WebUIControllerFactory);
};

}  // namespace brightray

#endif
