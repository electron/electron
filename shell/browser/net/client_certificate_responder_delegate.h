// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NET_CLIENT_CERTIFICATE_RESPONDER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_NET_CLIENT_CERTIFICATE_RESPONDER_DELEGATE_H_

#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader_network_service_observer.mojom-forward.h"

namespace content {
class BrowserContext;
}

namespace net {
class SSLCertRequestInfo;
}

namespace electron {

// Handles a client-certificate request that arrived over the network-service
// observer mojo pipe (i.e. from a non-WebContents URLLoader such as net.fetch
// or a utilityProcess request). Fetches matching identities from the platform
// store and routes the selection through ElectronBrowserClient so the
// `select-client-certificate` app event fires with a null WebContents.
void SelectClientCertificateForResponder(
    content::BrowserContext* browser_context,
    scoped_refptr<net::SSLCertRequestInfo> cert_info,
    mojo::PendingRemote<network::mojom::ClientCertificateResponder>
        cert_responder);

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NET_CLIENT_CERTIFICATE_RESPONDER_DELEGATE_H_
