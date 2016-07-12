// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
#define ATOM_BROWSER_API_ATOM_API_EXTENSION_H_

#include <string>
#include "atom/browser/atom_browser_context.h"
#include "atom/common/node_includes.h"
#include "base/memory/singleton.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "extensions/common/manifest.h"
#include "extensions/common/extension_set.h"

namespace base {
class FilePath;
}

namespace extensions {
class Extension;
}

namespace mate {
class Dictionary;
}

namespace atom {

namespace api {

class WebContents;

class Extension : public content::NotificationObserver {
 public:
  static Extension* GetInstance();

  static mate::Dictionary Load(v8::Isolate* isolate,
                    const base::FilePath& path,
                    const base::DictionaryValue& manifest,
                    const extensions::Manifest::Location& manifest_location,
                    int flags);
  static void Install(
      const scoped_refptr<const extensions::Extension>& extension);
  static void Disable(const std::string& extension_id);
  static void Enable(const std::string& extension_id);

  static bool HandleURLOverride(GURL* url,
                                     content::BrowserContext* browser_context);
  static bool HandleURLOverrideReverse(GURL* url,
                                     content::BrowserContext* browser_context);

  static bool IsBackgroundPageUrl(GURL url,
                                    content::BrowserContext* browser_context);
  static bool IsBackgroundPageWebContents(content::WebContents* web_contents);
  static bool IsBackgroundPage(WebContents* web_contents);

  static v8::Local<v8::Value> TabValue(v8::Isolate* isolate,
                                         WebContents* web_contents);
  const extensions::ExtensionSet& extensions() const { return extensions_; }

 private:
  friend struct base::DefaultSingletonTraits<Extension>;
  Extension();
  ~Extension();

  void Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) override;

  content::NotificationRegistrar registrar_;

  extensions::ExtensionSet extensions_;

  DISALLOW_COPY_AND_ASSIGN(Extension);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_EXTENSION_H_
