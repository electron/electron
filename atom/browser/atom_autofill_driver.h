// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_H_
#define ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_H_

#include <memory>

#if defined(TOOLKIT_VIEWS)
#include "atom/browser/ui/autofill_popup.h"
#endif

#include "atom/common/api/api.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace atom {

class AutofillDriver : public mojom::ElectronAutofillDriver {
 public:
  explicit AutofillDriver(content::RenderFrameHost* render_frame_host);

  ~AutofillDriver() override;

  void BindRequest(mojom::ElectronAutofillDriverAssociatedRequest request);

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

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_AUTOFILL_DRIVER_H_
