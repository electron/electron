// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_LANGUAGE_MODEL_H_
#define ELECTRON_SHELL_UTILITY_AI_UTILITY_AI_LANGUAGE_MODEL_H_

#include <list>
#include <vector>

#include "base/callback_list.h"
#include "base/memory/weak_ptr.h"
#include "gin/converter.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
#include "v8/include/v8.h"

namespace electron {

class UtilityAIManager;

class UtilityAILanguageModel : public blink::mojom::AILanguageModel {
 public:
  UtilityAILanguageModel(v8::Local<v8::Object> language_model,
                         base::WeakPtr<UtilityAIManager> manager);
  UtilityAILanguageModel(const UtilityAILanguageModel&) = delete;
  UtilityAILanguageModel& operator=(const UtilityAILanguageModel&) = delete;

  ~UtilityAILanguageModel() override;

  // Subscribe to be notified when this model is destroyed. The returned
  // subscription auto-unregisters when destroyed.
  [[nodiscard]] base::CallbackListSubscription AddDestroyObserver(
      base::RepeatingClosure callback);

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
  void OnResponderDisconnect(mojo::RemoteSetElementId id);

  blink::mojom::ModelStreamingResponder* GetResponder(
      mojo::RemoteSetElementId responder_id);

  base::WeakPtr<UtilityAIManager> manager_;
  v8::Global<v8::Object> language_model_;
  bool is_destroyed_ = false;

  mojo::RemoteSet<blink::mojom::ModelStreamingResponder> responder_set_;

  // Maps each in-progress Prompt/Append responder to its AbortController
  // so we can abort the JS-side operation if the responder disconnects.
  absl::flat_hash_map<mojo::RemoteSetElementId, v8::Global<v8::Object>>
      abort_controllers_;

  // Tracks abort controllers for in-progress MeasureInputUsage calls.
  std::list<v8::Global<v8::Object>> measure_abort_controllers_;

  // Notified when this model is destroyed, allowing in-progress
  // PromptResponder instances to clean up.
  base::RepeatingClosureList on_destroy_;

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
