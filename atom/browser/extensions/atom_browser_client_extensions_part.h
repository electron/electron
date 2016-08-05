// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_BROWSER_CLIENT_EXTENSIONS_PART_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_BROWSER_CLIENT_EXTENSIONS_PART_H_

#include <vector>
#include <string>
#include "base/compiler_specific.h"
#include "base/macros.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
class BrowserURLHandler;
class ResourceContext;
class SiteInstance;
class RenderProcessHost;
class RenderViewHost;
struct WebPreferences;
}

namespace extensions {

// Implements the extensions portion of AtomBrowserClient.
class AtomBrowserClientExtensionsPart {
 public:
  AtomBrowserClientExtensionsPart();
  ~AtomBrowserClientExtensionsPart();

  // Corresponds to the AtomBrowserClient function of the same name.
  static bool ShouldUseProcessPerSite(content::BrowserContext* profile,
                                      const GURL& effective_url);
  static bool ShouldSwapBrowsingInstancesForNavigation(
      content::SiteInstance* site_instance,
      const GURL& current_url,
      const GURL& new_url);

  // Similiar to AtomBrowserClient::ShouldAllowOpenURL(), but the
  // return value indicates whether to use |result| or not.
  static bool ShouldAllowOpenURL(content::SiteInstance* site_instance,
                                 const GURL& from_url,
                                 const GURL& to_url,
                                 bool* result);

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  static void SetApplicationLocale(std::string);

  void OverrideWebkitPrefs(content::RenderViewHost* host,
      content::WebPreferences* prefs);

  // // Helper function to call InfoMap::SetSigninProcess().
  // static void SetSigninProcess(content::SiteInstance* site_instance);
  void RenderProcessWillLaunch(content::RenderProcessHost* host);
  void SiteInstanceGotProcess(content::SiteInstance* site_instance);
  void SiteInstanceDeleting(content::SiteInstance* site_instance);
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler);
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_allowed_schemes);
  void AppendExtraRendererCommandLineSwitches(
      base::CommandLine* command_line,
      content::RenderProcessHost* process,
      content::BrowserContext* browser_context);
  std::string GetApplicationLocale();

 private:
  void UpdateContentSettings();
  void UpdateContentSettingsForHost(int render_process_id);


  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClientExtensionsPart);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_BROWSER_CLIENT_EXTENSIONS_PART_H_

