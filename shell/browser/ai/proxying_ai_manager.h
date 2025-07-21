// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_AI_PROXYING_AI_MANAGER_H_
#define ELECTRON_SHELL_BROWSER_AI_PROXYING_AI_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "content/public/browser/weak_document_ptr.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "shell/browser/session_preferences.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_manager.mojom.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
}  // namespace content

namespace electron {

// Owned by the host of the document / service worker via `SupportUserData`.
// The browser-side implementation of `blink::mojom::AIManager`, which
// proxies requests to a utility process if the session has a registered
// handler.
class ProxyingAIManager : public base::SupportsUserData::Data,
                          public blink::mojom::AIManager {
 public:
  ProxyingAIManager(content::BrowserContext* browser_context,
                    content::RenderFrameHost* rfh);
  ProxyingAIManager(const ProxyingAIManager&) = delete;
  ProxyingAIManager& operator=(const ProxyingAIManager&) = delete;

  ~ProxyingAIManager() override;

  void AddReceiver(mojo::PendingReceiver<blink::mojom::AIManager> receiver);

 private:
  // Lazily bind the AIManager remote so that the developer can
  // set the local AI handler after this class is already created
  [[nodiscard]] const mojo::Remote<blink::mojom::AIManager>& GetAIManagerRemote(
      const SessionPreferences& session_prefs);

  // `blink::mojom::AIManager` implementation.
  void CanCreateLanguageModel(
      blink::mojom::AILanguageModelCreateOptionsPtr options,
      CanCreateLanguageModelCallback callback) override;
  void CreateLanguageModel(
      mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
          client,
      blink::mojom::AILanguageModelCreateOptionsPtr options) override;
  void CanCreateSummarizer(blink::mojom::AISummarizerCreateOptionsPtr options,
                           CanCreateSummarizerCallback callback) override;
  void CreateSummarizer(
      mojo::PendingRemote<blink::mojom::AIManagerCreateSummarizerClient> client,
      blink::mojom::AISummarizerCreateOptionsPtr options) override;
  void GetLanguageModelParams(GetLanguageModelParamsCallback callback) override;
  void CanCreateWriter(blink::mojom::AIWriterCreateOptionsPtr options,
                       CanCreateWriterCallback callback) override;
  void CreateWriter(
      mojo::PendingRemote<blink::mojom::AIManagerCreateWriterClient> client,
      blink::mojom::AIWriterCreateOptionsPtr options) override;
  void CanCreateRewriter(blink::mojom::AIRewriterCreateOptionsPtr options,
                         CanCreateRewriterCallback callback) override;
  void CreateRewriter(
      mojo::PendingRemote<blink::mojom::AIManagerCreateRewriterClient> client,
      blink::mojom::AIRewriterCreateOptionsPtr options) override;
  void CanCreateProofreader(blink::mojom::AIProofreaderCreateOptionsPtr options,
                            CanCreateProofreaderCallback callback) override;
  void CreateProofreader(
      mojo::PendingRemote<blink::mojom::AIManagerCreateProofreaderClient>
          client,
      blink::mojom::AIProofreaderCreateOptionsPtr options) override;
  void AddModelDownloadProgressObserver(
      mojo::PendingRemote<blink::mojom::ModelDownloadProgressObserver>
          observer_remote) override;

  mojo::ReceiverSet<blink::mojom::AIManager> receivers_;

  raw_ptr<content::BrowserContext> browser_context_;

  content::WeakDocumentPtr rfh_;

  // TODO - How to handle the case where the developer has switched to a
  // different handler?
  mojo::Remote<blink::mojom::AIManager> ai_manager_remote_;

  // TODO(dsanders11) - This is unused at the moment
  base::WeakPtrFactory<ProxyingAIManager> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_AI_PROXYING_AI_MANAGER_H_
