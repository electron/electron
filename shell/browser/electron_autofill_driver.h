// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_
#define SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_

#include <memory>
#include <vector>

#if defined(TOOLKIT_VIEWS)
#include "shell/browser/ui/autofill_popup.h"
#endif

#include "mojo/public/cpp/bindings/associated_binding.h"
#include "shell/common/api/api.mojom.h"

namespace electron {

class AutofillDriver : public mojom::ElectronAutofillDriver {
 public:
  AutofillDriver(content::RenderFrameHost* render_frame_host,
                 mojom::ElectronAutofillDriverAssociatedRequest request);

  ~AutofillDriver() override;

  void ShowAutofillPopup(const gfx::RectF& bounds,
                         const std::vector<base::string16>& values,
                         const std::vector<base::string16>& labels) override;
  void HideAutofillPopup() override;

 private:
  content::RenderFrameHost* const render_frame_host_;

#if defined(TOOLKIT_VIEWS)
  std::unique_ptr<AutofillPopup> autofill_popup_;
#endif

  mojo::AssociatedBinding<mojom::ElectronAutofillDriver> binding_;
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_
