// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_MANAGER_H_
#define ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_MANAGER_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "services/on_device_model/public/mojom/download_observer.mojom-forward.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom-forward.h"
#include "third_party/blink/public/mojom/ai/ai_manager.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_proofreader.mojom-forward.h"
#include "third_party/blink/public/mojom/ai/ai_rewriter.mojom-forward.h"
#include "third_party/blink/public/mojom/ai/ai_summarizer.mojom-forward.h"
#include "third_party/blink/public/mojom/ai/ai_writer.mojom-forward.h"
#include "url/origin.h"
#include "v8/include/v8.h"

namespace electron {

class UtilityAILanguageModel;

// The utility-side implementation of `blink::mojom::AIManager`.
class UtilityAIManager : public blink::mojom::AIManager {
 public:
  UtilityAIManager(std::optional<int32_t> web_contents_id,
                   const url::Origin& security_origin);
  UtilityAIManager(const UtilityAIManager&) = delete;
  UtilityAIManager& operator=(const UtilityAIManager&) = delete;

  ~UtilityAIManager() override;

 private:
  friend class UtilityAILanguageModel;

  void OnCreateLanguageModelClientDisconnect(mojo::RemoteSetElementId id,
                                             uint32_t custom_reason,
                                             const std::string& description);
  [[nodiscard]] v8::Global<v8::Object>& GetLanguageModelClass();

  void SendCreateLanguageModelError(
      mojo::RemoteSetElementId client_id,
      blink::mojom::AIManagerCreateClientError error);

  void CreateLanguageModelInternal(
      v8::Isolate* isolate,
      mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
          client,
      v8::Local<v8::Object> target,
      std::string_view method_name,
      gin_helper::Dictionary options_dict,
      blink::mojom::AILanguageModelCreateOptionsPtr options);

  void HandleLanguageModelResult(
      v8::Isolate* isolate,
      v8::Local<v8::Object> language_model,
      mojo::RemoteSetElementId client_id,
      blink::mojom::AILanguageModelCreateOptionsPtr options);

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
      mojo::PendingRemote<on_device_model::mojom::DownloadObserver>
          observer_remote) override;

  std::optional<int32_t> web_contents_id_;
  url::Origin security_origin_;

  v8::Global<v8::Object> language_model_class_;

  mojo::RemoteSet<blink::mojom::AIManagerCreateLanguageModelClient>
      create_model_client_set_;

  // Maps each in-progress CreateLanguageModel client to its AbortController
  // so we can abort the JS-side operation if the client disconnects.
  absl::flat_hash_map<mojo::RemoteSetElementId, v8::Global<v8::Object>>
      abort_controllers_;

  // Owns all created UtilityAILanguageModel instances
  mojo::UniqueReceiverSet<blink::mojom::AILanguageModel>
      language_model_receivers_;

  base::WeakPtrFactory<UtilityAIManager> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_MANAGER_H_
