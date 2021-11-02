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

#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"

namespace electron {

class AutofillDriver : public mojom::ElectronAutofillDriver {
 public:
  AutofillDriver(
#if 0  // TESTING(ckerr)
      content::RenderFrameHost* render_frame_host,
      mojo::PendingAssociatedReceiver<mojom::ElectronAutofillDriver> request);
#else
      content::RenderFrameHost* render_frame_host);
#endif

  ~AutofillDriver() override;

  void ShowAutofillPopup(const gfx::RectF& bounds,
                         const std::vector<std::u16string>& values,
                         const std::vector<std::u16string>& labels) override;
  void HideAutofillPopup() override;

 private:
  content::RenderFrameHost* const render_frame_host_;

#if defined(TOOLKIT_VIEWS)
  std::unique_ptr<AutofillPopup> autofill_popup_;
#endif

#if 0  // TESTING(ckerr)
  mojo::AssociatedReceiver<mojom::ElectronAutofillDriver> receiver_;
#endif
};

}  // namespace electron

#endif  // SHELL_BROWSER_ELECTRON_AUTOFILL_DRIVER_H_
