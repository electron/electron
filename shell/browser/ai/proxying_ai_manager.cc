// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ai/proxying_ai_manager.h"

#include <optional>
#include <utility>

#include "base/notimplemented.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/weak_document_ptr.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/session_preferences.h"
#include "third_party/blink/public/mojom/ai/ai_common.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"

namespace electron {

ProxyingAIManager::ProxyingAIManager(content::BrowserContext* browser_context,
                                     content::RenderFrameHost* rfh)
    : browser_context_(browser_context),
      rfh_(rfh ? rfh->GetWeakDocumentPtr() : content::WeakDocumentPtr()) {}

ProxyingAIManager::~ProxyingAIManager() = default;

void ProxyingAIManager::AddReceiver(
    mojo::PendingReceiver<blink::mojom::AIManager> receiver) {
  receivers_.Add(this, std::move(receiver));
}

const mojo::Remote<blink::mojom::AIManager>&
ProxyingAIManager::GetAIManagerRemote(const SessionPreferences& session_prefs) {
  if (!ai_manager_remote_.is_bound()) {
    auto* local_ai_handler = session_prefs.GetLocalAIHandler().get();

    if (local_ai_handler) {
      auto* rfh = rfh_.AsRenderFrameHostIfValid();
      DCHECK(rfh);

      auto* web_contents = electron::api::WebContents::From(
          content::WebContents::FromRenderFrameHost(rfh));
      std::optional<int32_t> web_contents_id;

      if (web_contents) {
        web_contents_id = web_contents->ID();
      }

      local_ai_handler->BindAIManager(
          web_contents_id, rfh->GetLastCommittedOrigin(),
          ai_manager_remote_.BindNewPipeAndPassReceiver());
    }
  } else {
    // Developer may have unregistered the local AI handler
    // TODO - This should really happen the moment they unregister
    //        it so that in-progress calls to it get killed off
    auto* local_ai_handler = session_prefs.GetLocalAIHandler().get();

    if (!local_ai_handler) {
      ai_manager_remote_.reset();
    }
  }

  return ai_manager_remote_;
}

void ProxyingAIManager::CanCreateLanguageModel(
    blink::mojom::AILanguageModelCreateOptionsPtr options,
    CanCreateLanguageModelCallback callback) {
  auto* session_prefs =
      SessionPreferences::FromBrowserContext(browser_context_);
  DCHECK(session_prefs);

  // Default to unavailable. This ensures the callback is always invoked
  // even if there is no registered utility process handler, or the
  // process crashes.
  auto cb = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback),
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);

  // Proxy the call through to the utility process
  auto& ai_manager = GetAIManagerRemote(*session_prefs);

  if (ai_manager.is_bound()) {
    ai_manager->CanCreateLanguageModel(std::move(options), std::move(cb));
  }
}

void ProxyingAIManager::CreateLanguageModel(
    mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
        client,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  auto* session_prefs =
      SessionPreferences::FromBrowserContext(browser_context_);
  DCHECK(session_prefs);

  // Proxy the call through to the utility process
  auto& ai_manager = GetAIManagerRemote(*session_prefs);

  if (!ai_manager.is_bound()) {
    mojo::Remote<blink::mojom::AIManagerCreateLanguageModelClient>
        client_remote(std::move(client));
    client_remote->OnError(
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession,
        /*quota_error_info=*/nullptr);
    return;
  }

  ai_manager->CreateLanguageModel(std::move(client), std::move(options));

  // TODO - Implement language model creation logic
  // TODO - Does there need to be a proxying AILanguageModel impl?
}

void ProxyingAIManager::CanCreateSummarizer(
    blink::mojom::AISummarizerCreateOptionsPtr options,
    CanCreateSummarizerCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void ProxyingAIManager::CreateSummarizer(
    mojo::PendingRemote<blink::mojom::AIManagerCreateSummarizerClient> client,
    blink::mojom::AISummarizerCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void ProxyingAIManager::GetLanguageModelParams(
    GetLanguageModelParamsCallback callback) {
  auto* session_prefs =
      SessionPreferences::FromBrowserContext(browser_context_);
  DCHECK(session_prefs);

  // Default to null. This ensures the callback is always invoked
  // even if there is no registered utility process handler, or the
  // process crashes.
  auto cb =
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback), nullptr);

  // Proxy the call through to the utility process
  auto& ai_manager = GetAIManagerRemote(*session_prefs);

  if (ai_manager.is_bound()) {
    ai_manager->GetLanguageModelParams(std::move(cb));
  }
}

void ProxyingAIManager::CanCreateWriter(
    blink::mojom::AIWriterCreateOptionsPtr options,
    CanCreateWriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void ProxyingAIManager::CreateWriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateWriterClient> client,
    blink::mojom::AIWriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void ProxyingAIManager::CanCreateRewriter(
    blink::mojom::AIRewriterCreateOptionsPtr options,
    CanCreateRewriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void ProxyingAIManager::CreateRewriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateRewriterClient> client,
    blink::mojom::AIRewriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void ProxyingAIManager::CanCreateProofreader(
    blink::mojom::AIProofreaderCreateOptionsPtr options,
    CanCreateProofreaderCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void ProxyingAIManager::CreateProofreader(
    mojo::PendingRemote<blink::mojom::AIManagerCreateProofreaderClient> client,
    blink::mojom::AIProofreaderCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void ProxyingAIManager::AddModelDownloadProgressObserver(
    mojo::PendingRemote<blink::mojom::ModelDownloadProgressObserver>
        observer_remote) {
  NOTIMPLEMENTED();
}

}  // namespace electron
