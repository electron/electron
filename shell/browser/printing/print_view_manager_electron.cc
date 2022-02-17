// Copyright 2020 Microsoft, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/printing/print_view_manager_electron.h"

#include <utility>

#include "build/build_config.h"
#include "content/public/browser/web_contents_user_data.h"

namespace electron {

PrintViewManagerElectron::PrintViewManagerElectron(
    content::WebContents* web_contents)
    : PrintViewManagerBase(web_contents),
      content::WebContentsUserData<PrintViewManagerElectron>(*web_contents) {}

PrintViewManagerElectron::~PrintViewManagerElectron() = default;

// static
void PrintViewManagerElectron::BindPrintManagerHost(
    mojo::PendingAssociatedReceiver<printing::mojom::PrintManagerHost> receiver,
    content::RenderFrameHost* rfh) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents)
    return;
  auto* print_manager = PrintViewManagerElectron::FromWebContents(web_contents);
  if (!print_manager)
    return;
  print_manager->BindReceiver(std::move(receiver), rfh);
}

void PrintViewManagerElectron::SetupScriptedPrintPreview(
    SetupScriptedPrintPreviewCallback callback) {
  std::move(callback).Run();
}

void PrintViewManagerElectron::ShowScriptedPrintPreview(
    bool source_is_modifiable) {}

void PrintViewManagerElectron::RequestPrintPreview(
    printing::mojom::RequestPrintPreviewParamsPtr params) {}

void PrintViewManagerElectron::CheckForCancel(int32_t preview_ui_id,
                                              int32_t request_id,
                                              CheckForCancelCallback callback) {
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(PrintViewManagerElectron);

}  // namespace electron
