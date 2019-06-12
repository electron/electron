// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_FACTORY_H_
#define ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_FACTORY_H_

#include <memory>
#include <unordered_map>

#include "base/callback_forward.h"
#include "base/supports_user_data.h"
#include "content/public/browser/web_contents_observer.h"
#include "electron/atom/common/api/api.mojom.h"

namespace atom {

class AutofillDriver;

class AutofillDriverFactory : public content::WebContentsObserver,
                              public base::SupportsUserData::Data {
 public:
  explicit AutofillDriverFactory(content::WebContents* web_contents);

  ~AutofillDriverFactory() override;

  static void CreateForWebContents(content::WebContents* contents);

  static AutofillDriverFactory* FromWebContents(content::WebContents* contents);
  static void BindAutofillDriver(
      mojom::ElectronAutofillDriverAssociatedRequest request,
      content::RenderFrameHost* render_frame_host);

  // content::WebContentsObserver:
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  AutofillDriver* DriverForFrame(content::RenderFrameHost* render_frame_host);
  void AddDriverForFrame(
      content::RenderFrameHost* render_frame_host,
      base::Callback<std::unique_ptr<AutofillDriver>()> factory_method);
  void DeleteDriverForFrame(content::RenderFrameHost* render_frame_host);

  void CloseAllPopups();

  static const char kAtomAutofillDriverFactoryWebContentsUserDataKey[];

 private:
  std::unordered_map<content::RenderFrameHost*, std::unique_ptr<AutofillDriver>>
      driver_map_;
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_FACTORY_H_
