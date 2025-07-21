// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UTILITY_PROCESS_AI_LANGUAGE_MODEL_H_
#define ELECTRON_SHELL_BROWSER_UTILITY_PROCESS_AI_LANGUAGE_MODEL_H_

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"

namespace electron {

// The implementation of `blink::mojom::AILanguageModel` which only echoes
// back the prompt text used for testing.
class UtilityProcessAILanguageModel : public blink::mojom::AILanguageModel {
 public:
  explicit UtilityProcessAILanguageModel(
      blink::mojom::AILanguageModelSamplingParamsPtr sampling_params,
      base::flat_set<blink::mojom::AILanguageModelPromptType> input_types,
      uint32_t initial_tokens_size);
  UtilityProcessAILanguageModel(const UtilityProcessAILanguageModel&) = delete;
  UtilityProcessAILanguageModel& operator=(
      const UtilityProcessAILanguageModel&) = delete;

  ~UtilityProcessAILanguageModel() override;

  // `blink::mojom::AILanguageModel` implementation.
  void Prompt(std::vector<blink::mojom::AILanguageModelPromptPtr> prompts,
              on_device_model::mojom::ResponseConstraintPtr constraint,
              mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
                  pending_responder) override;
  void Append(std::vector<blink::mojom::AILanguageModelPromptPtr> prompts,
              mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
                  pending_responder) override;
  void Fork(
      mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
          client) override;
  void Destroy() override;
  void MeasureInputUsage(
      std::vector<blink::mojom::AILanguageModelPromptPtr> input,
      MeasureInputUsageCallback callback) override;

 private:
  void DoMockExecution(const std::string& input,
                       mojo::RemoteSetElementId responder_id);

  bool is_destroyed_ = false;
  uint64_t current_tokens_ = 0;
  blink::mojom::AILanguageModelSamplingParamsPtr sampling_params_;
  // Prompt types supported by the language model in this session.
  base::flat_set<blink::mojom::AILanguageModelPromptType> input_types_;

  mojo::RemoteSet<blink::mojom::ModelStreamingResponder> responder_set_;

  base::WeakPtrFactory<UtilityProcessAILanguageModel> weak_ptr_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_UTILITY_PROCESS_AI_LANGUAGE_MODEL_H_
