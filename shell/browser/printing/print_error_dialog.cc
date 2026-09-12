// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/print_error_dialog.h"  // nogncheck

#include "base/logging.h"

// Electron reports print failures through webContents.print()'s callback
// rather than Chromium's dialogs; //chrome/browser/printing references these.

void ShowPrintErrorDialogForInvalidPrinterError() {
  LOG(ERROR) << "The selected printer is not available or not installed "
                "correctly.";
}

void ShowPrintErrorDialogForGenericError() {
  LOG(ERROR) << "Printing failed.";
}
