// Copyright (c) 2026 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/client_certificate_responder_delegate.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/ssl/client_cert_identity.h"
#include "net/ssl/client_cert_store.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_private_key.h"
#include "services/network/public/mojom/url_loader_network_service_observer.mojom.h"
#include "shell/browser/electron_browser_client.h"

namespace electron {

namespace {

// Mirrors content/browser/ssl_private_key_impl.h (not public) so the network
// service can sign with a key that lives in the browser process.
class SSLPrivateKeyImpl : public network::mojom::SSLPrivateKey {
 public:
  explicit SSLPrivateKeyImpl(scoped_refptr<net::SSLPrivateKey> ssl_private_key)
      : ssl_private_key_(std::move(ssl_private_key)) {}

  SSLPrivateKeyImpl(const SSLPrivateKeyImpl&) = delete;
  SSLPrivateKeyImpl& operator=(const SSLPrivateKeyImpl&) = delete;

  // network::mojom::SSLPrivateKey:
  void Sign(uint16_t algorithm,
            const std::vector<uint8_t>& input,
            SignCallback callback) override {
    ssl_private_key_->Sign(
        algorithm, input,
        base::BindOnce(
            [](SignCallback callback, net::Error net_error,
               const std::vector<uint8_t>& signature) {
              std::move(callback).Run(static_cast<int32_t>(net_error),
                                      signature);
            },
            std::move(callback)));
  }

 private:
  scoped_refptr<net::SSLPrivateKey> ssl_private_key_;
};

// Adapts the mojo responder to the content::ClientCertificateDelegate
// interface that ElectronBrowserClient::SelectClientCertificate expects.
class ClientCertificateResponderDelegate
    : public content::ClientCertificateDelegate {
 public:
  explicit ClientCertificateResponderDelegate(
      mojo::PendingRemote<network::mojom::ClientCertificateResponder> responder)
      : responder_(std::move(responder)) {
    responder_.set_disconnect_handler(base::BindOnce(
        &ClientCertificateResponderDelegate::OnDisconnect,
        base::Unretained(this)));
  }

  ~ClientCertificateResponderDelegate() override {
    if (!responded_ && responder_)
      responder_->CancelRequest();
  }

  ClientCertificateResponderDelegate(
      const ClientCertificateResponderDelegate&) = delete;
  ClientCertificateResponderDelegate& operator=(
      const ClientCertificateResponderDelegate&) = delete;

  // content::ClientCertificateDelegate:
  void ContinueWithCertificate(
      scoped_refptr<net::X509Certificate> cert,
      scoped_refptr<net::SSLPrivateKey> private_key) override {
    if (responded_ || !responder_)
      return;
    responded_ = true;
    if (!cert || !private_key) {
      responder_->ContinueWithoutCertificate();
      return;
    }
    mojo::PendingRemote<network::mojom::SSLPrivateKey> ssl_private_key;
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<SSLPrivateKeyImpl>(private_key),
        ssl_private_key.InitWithNewPipeAndPassReceiver());
    responder_->ContinueWithCertificate(
        std::move(cert), private_key->GetProviderName(),
        private_key->GetAlgorithmPreferences(), std::move(ssl_private_key));
  }

 private:
  void OnDisconnect() { responder_.reset(); }

  bool responded_ = false;
  mojo::Remote<network::mojom::ClientCertificateResponder> responder_;
};

void OnGotClientCerts(
    std::unique_ptr<net::ClientCertStore> cert_store,
    std::unique_ptr<content::ClientCertificateDelegate> delegate,
    content::BrowserContext* browser_context,
    scoped_refptr<net::SSLCertRequestInfo> cert_info,
    net::ClientCertIdentityList identities) {
  // ElectronBrowserClient handles both the empty-list case (continue without a
  // certificate) and routing to the `select-client-certificate` app event.
  ElectronBrowserClient::Get()->SelectClientCertificate(
      browser_context, /*process_id=*/0, /*web_contents=*/nullptr,
      cert_info.get(), std::move(identities), std::move(delegate));
}

}  // namespace

void SelectClientCertificateForResponder(
    content::BrowserContext* browser_context,
    scoped_refptr<net::SSLCertRequestInfo> cert_info,
    mojo::PendingRemote<network::mojom::ClientCertificateResponder>
        cert_responder) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto delegate = std::make_unique<ClientCertificateResponderDelegate>(
      std::move(cert_responder));

  std::unique_ptr<net::ClientCertStore> cert_store =
      ElectronBrowserClient::Get()->CreateClientCertStore(browser_context);
  if (!cert_store) {
    delegate->ContinueWithCertificate(nullptr, nullptr);
    return;
  }

  // The store must outlive the async lookup; bind it into the callback.
  auto* cert_store_ptr = cert_store.get();
  cert_store_ptr->GetClientCerts(
      cert_info,
      base::BindOnce(&OnGotClientCerts, std::move(cert_store),
                     std::move(delegate), browser_context, cert_info));
}

}  // namespace electron
