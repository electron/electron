// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ai/ai_manager.h"

#include <utility>

#include "base/notimplemented.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/weak_document_ptr.h"
#include "shell/browser/api/electron_api_session.h"
#include "third_party/blink/public/mojom/ai/ai_common.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"

namespace electron {

AIManager::AIManager(content::BrowserContext* browser_context,
                     content::RenderFrameHost* rfh)
    : browser_context_(browser_context),
      rfh_(rfh ? rfh->GetWeakDocumentPtr() : content::WeakDocumentPtr()) {}

AIManager::~AIManager() = default;

void AIManager::AddReceiver(
    mojo::PendingReceiver<blink::mojom::AIManager> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void AIManager::CanCreateLanguageModel(
    blink::mojom::AILanguageModelCreateOptionsPtr options,
    CanCreateLanguageModelCallback callback) {
  // TODO - Check if there's a registered utility process for AI stuff
  auto* session = api::Session::FromBrowserContext(browser_context_);

  if (session) {
    std::move(callback).Run(
        blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
  } else {
    std::move(callback).Run(
        blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
  }
}

void AIManager::CreateLanguageModel(
    mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
        client,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  // TODO - Implement language model creation logic
}

void AIManager::CanCreateSummarizer(
    blink::mojom::AISummarizerCreateOptionsPtr options,
    CanCreateSummarizerCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void AIManager::CreateSummarizer(
    mojo::PendingRemote<blink::mojom::AIManagerCreateSummarizerClient> client,
    blink::mojom::AISummarizerCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void AIManager::GetLanguageModelParams(
    GetLanguageModelParamsCallback callback) {
  // TODO - Implement this
}

void AIManager::CanCreateWriter(blink::mojom::AIWriterCreateOptionsPtr options,
                                CanCreateWriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void AIManager::CreateWriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateWriterClient> client,
    blink::mojom::AIWriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void AIManager::CanCreateRewriter(
    blink::mojom::AIRewriterCreateOptionsPtr options,
    CanCreateRewriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void AIManager::CreateRewriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateRewriterClient> client,
    blink::mojom::AIRewriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void AIManager::CanCreateProofreader(
    blink::mojom::AIProofreaderCreateOptionsPtr options,
    CanCreateProofreaderCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void AIManager::CreateProofreader(
    mojo::PendingRemote<blink::mojom::AIManagerCreateProofreaderClient> client,
    blink::mojom::AIProofreaderCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void AIManager::AddModelDownloadProgressObserver(
    mojo::PendingRemote<blink::mojom::ModelDownloadProgressObserver>
        observer_remote) {
  NOTIMPLEMENTED();
};

}  // namespace electron
