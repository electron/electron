// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_LANGUAGE_MODEL_H_
#define ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_LANGUAGE_MODEL_H_

#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "gin/converter.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
#include "v8/include/v8.h"

namespace electron {

class UtilityAILanguageModel : public blink::mojom::AILanguageModel {
 public:
  UtilityAILanguageModel(v8::Local<v8::Object> language_model);
  UtilityAILanguageModel(const UtilityAILanguageModel&) = delete;
  UtilityAILanguageModel& operator=(const UtilityAILanguageModel&) = delete;

  ~UtilityAILanguageModel() override;

  static bool IsLanguageModel(v8::Isolate* isolate, v8::Local<v8::Value> val);
  static bool IsLanguageModelClass(v8::Isolate* isolate,
                                   v8::Local<v8::Value> val);

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
  blink::mojom::ModelStreamingResponder* GetResponder(
      mojo::RemoteSetElementId responder_id);
  blink::mojom::AIManagerCreateLanguageModelClient*
  GetCreateLanguageModelClient(mojo::RemoteSetElementId responder_id);

  v8::Global<v8::Object> language_model_;
  bool is_destroyed_ = false;

  mojo::RemoteSet<blink::mojom::ModelStreamingResponder> responder_set_;
  mojo::RemoteSet<blink::mojom::AIManagerCreateLanguageModelClient>
      create_model_client_set_;

  base::WeakPtrFactory<UtilityAILanguageModel> weak_ptr_factory_{this};
};

}  // namespace electron

namespace gin {

template <>
struct Converter<blink::mojom::AILanguageModelPromptPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::AILanguageModelPromptPtr& val);
};

}  // namespace gin

#endif  // ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_LANGUAGE_MODEL_H_
