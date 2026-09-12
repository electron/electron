// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/certificate_viewer.h"

#include <windows.h>  // windows.h must be included first

#include <cryptuiapi.h>

#include <utility>

#include "base/functional/bind.h"
#include "crypto/scoped_capi_types.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_win.h"
#include "shell/browser/ui/win/dialog_thread.h"
#include "ui/views/win/hwnd_util.h"

namespace electron {

namespace {

bool ShowViewerOnDialogThread(scoped_refptr<net::X509Certificate> cert,
                              HWND owner) {
  // Create a cert context and store containing the certificate and its
  // intermediates.
  crypto::ScopedPCCERT_CONTEXT cert_context(
      net::x509_util::CreateCertContextWithChain(cert.get()));
  if (!cert_context)
    return false;

  CRYPTUI_VIEWCERTIFICATE_STRUCT view_info = {0};
  view_info.dwSize = sizeof(view_info);
  view_info.hwndParent = owner;
  view_info.dwFlags =
      CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE;
  view_info.pCertContext = cert_context.get();
  HCERTSTORE cert_store = cert_context->hCertStore;
  view_info.cStores = 1;
  view_info.rghStores = &cert_store;

  BOOL properties_changed;
  ::CryptUIDlgViewCertificate(&view_info, &properties_changed);
  return true;
}

}  // namespace

void ShowPlatformCertificateViewer(net::X509Certificate* cert,
                                   gfx::NativeWindow parent) {
  HWND parent_hwnd = parent ? views::HWNDForNativeWindow(parent) : nullptr;

  // CryptUIDlgViewCertificate is a blocking call, so run it on the dedicated
  // dialog thread like Electron's other native Windows dialogs.
  dialog_thread::Run(
      base::BindOnce(&ShowViewerOnDialogThread,
                     scoped_refptr<net::X509Certificate>(cert), parent_hwnd),
      base::BindOnce([](bool shown) {}));
}

}  // namespace electron
