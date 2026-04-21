// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/ai/utility_ai_manager.h"

#include <optional>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/notimplemented.h"
#include "mojo/public/cpp/bindings/unique_receiver_set.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/utility/ai/utility_ai_language_model.h"
#include "shell/utility/api/electron_api_local_ai_handler.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"
#include "third_party/blink/public/mojom/ai/ai_common.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_proofreader.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_rewriter.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_summarizer.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_writer.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "v8/include/v8.h"

namespace gin {

template <>
struct Converter<blink::mojom::ModelAvailabilityCheckResult> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::ModelAvailabilityCheckResult* out) {
    using Result = blink::mojom::ModelAvailabilityCheckResult;
    static constexpr auto Lookup =
        base::MakeFixedFlatMap<std::string_view, Result>({
            {"available", Result::kAvailable},
            {"unavailable", Result::kUnavailableUnknown},
            {"downloading", Result::kDownloading},
            {"downloadable", Result::kDownloadable},
        });
    return FromV8WithLookup(isolate, val, Lookup, out);
  }
};

template <>
struct Converter<blink::mojom::AILanguageModelPromptType> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      blink::mojom::AILanguageModelPromptType value) {
    switch (value) {
      case blink::mojom::AILanguageModelPromptType::kText:
        return StringToV8(isolate, "text");
      case blink::mojom::AILanguageModelPromptType::kImage:
        return StringToV8(isolate, "image");
      case blink::mojom::AILanguageModelPromptType::kAudio:
        return StringToV8(isolate, "audio");
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

template <>
struct Converter<blink::mojom::AILanguageCodePtr> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const blink::mojom::AILanguageCodePtr& val) {
    if (val.is_null()) {
      return v8::Undefined(isolate);
    }

    return StringToV8(isolate, val->code);
  }
};

template <>
struct Converter<blink::mojom::AILanguageModelExpectedPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::AILanguageModelExpectedPtr& val) {
    if (val.is_null()) {
      return v8::Undefined(isolate);
    }

    auto dict = gin::Dictionary::CreateEmpty(isolate);

    dict.Set("type", val->type);

    if (val->languages.has_value() && !val->languages->empty()) {
      dict.Set("languages", val->languages.value());
    }

    return ConvertToV8(isolate, dict);
  }
};

template <>
struct Converter<blink::mojom::AILanguageModelCreateOptionsPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::AILanguageModelCreateOptionsPtr& val) {
    if (val.is_null() ||
        (val->sampling_params.is_null() && !val->expected_inputs.has_value() &&
         !val->expected_outputs.has_value() && val->initial_prompts.empty())) {
      return v8::Undefined(isolate);
    }

    auto dict = gin::Dictionary::CreateEmpty(isolate);

    if (val->expected_inputs.has_value() && !val->expected_inputs->empty()) {
      dict.Set("expectedInputs", val->expected_inputs.value());
    }

    if (val->expected_outputs.has_value() && !val->expected_outputs->empty()) {
      dict.Set("expectedOutputs", val->expected_outputs.value());
    }

    if (!val->initial_prompts.empty()) {
      dict.Set("initialPrompts", val->initial_prompts);
    }

    return ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

UtilityAIManager::UtilityAIManager(std::optional<int32_t> web_contents_id,
                                   const url::Origin& security_origin)
    : web_contents_id_(web_contents_id), security_origin_(security_origin) {
  create_model_client_set_.set_disconnect_with_reason_handler(
      base::BindRepeating(
          &UtilityAIManager::OnCreateLanguageModelClientDisconnect,
          weak_ptr_factory_.GetWeakPtr()));
}

UtilityAIManager::~UtilityAIManager() {
  // Trigger the abort signal for any in-progress CreateLanguageModel calls
  for (auto it = create_model_client_set_.begin();
       it != create_model_client_set_.end(); ++it) {
    OnCreateLanguageModelClientDisconnect(it.id(), 0, std::string());
  }
}

void UtilityAIManager::OnCreateLanguageModelClientDisconnect(
    mojo::RemoteSetElementId id,
    uint32_t custom_reason,
    const std::string& description) {
  auto it = abort_controllers_.find(id);
  if (it != abort_controllers_.end()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope{isolate};
    if (description.empty()) {
      gin_helper::CallMethod(isolate, it->second.Get(isolate), "abort");
    } else {
      gin_helper::CallMethod(isolate, it->second.Get(isolate), "abort",
                             description);
    }
    abort_controllers_.erase(it);
  }
}

v8::Global<v8::Object>& UtilityAIManager::GetLanguageModelClass() {
  if (language_model_class_.IsEmpty()) {
    auto& handler = electron::api::local_ai_handler::GetPromptAPIHandler();

    if (handler.has_value()) {
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      v8::HandleScope scope{isolate};

      auto details = gin_helper::Dictionary::CreateEmpty(isolate);
      if (web_contents_id_.has_value()) {
        details.Set("webContentsId", web_contents_id_.value());
      } else {
        details.Set("webContentsId", nullptr);
      }
      details.Set("securityOrigin", security_origin_.Serialize());

      v8::Local<v8::Value> val = handler->Run(details);

      if (val->IsPromise()) {
        auto err = v8::Exception::TypeError(gin::StringToV8(
            isolate, "Cannot return a promise from the handler"));
        node::errors::TriggerUncaughtException(isolate, err, {});
        return language_model_class_;
      }

      if (val->IsNull()) {
        // The handler chose not to provide a class.
        return language_model_class_;
      } else if (!val->IsObject() ||
                 !val->ToObject(isolate->GetCurrentContext())
                      .ToLocalChecked()
                      ->IsConstructor() ||
                 !UtilityAILanguageModel::IsLanguageModelClass(isolate, val)) {
        auto err = v8::Exception::TypeError(
            gin::StringToV8(isolate, "Must provide a constructible class"));
        node::errors::TriggerUncaughtException(isolate, err, {});
        return language_model_class_;
      } else {
        language_model_class_.Reset(
            isolate,
            val->ToObject(isolate->GetCurrentContext()).ToLocalChecked());
      }
    }
  }

  return language_model_class_;
}

void UtilityAIManager::SendCreateLanguageModelError(
    mojo::RemoteSetElementId client_id,
    blink::mojom::AIManagerCreateClientError error) {
  abort_controllers_.erase(client_id);

  blink::mojom::AIManagerCreateLanguageModelClient* client =
      create_model_client_set_.Get(client_id);
  if (!client) {
    return;
  }

  client->OnError(error, /*quota_error_info=*/nullptr);
}

void UtilityAIManager::HandleLanguageModelResult(
    v8::Isolate* isolate,
    v8::Local<v8::Object> language_model,
    mojo::RemoteSetElementId client_id,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  abort_controllers_.erase(client_id);

  gin_helper::Dictionary dict;
  uint64_t context_usage = 0;
  uint64_t context_quota = 0;

  if (!ConvertFromV8(isolate, language_model, &dict) ||
      !dict.Get("contextUsage", &context_usage) ||
      !dict.Get("contextWindow", &context_quota)) {
    SendCreateLanguageModelError(
        client_id,
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession);
    return;
  }

  // TODO - How can the implementation specify the supported prompt types? For
  // now, assume all types are supported if the handler returns a valid object
  base::flat_set<blink::mojom::AILanguageModelPromptType> enabled_input_types;
  if (options->expected_inputs.has_value()) {
    for (const auto& expected_input : options->expected_inputs.value()) {
      enabled_input_types.insert(expected_input->type);
    }
  }

  blink::mojom::AIManagerCreateLanguageModelClient* client =
      create_model_client_set_.Get(client_id);
  if (!client) {
    return;
  }

  mojo::PendingRemote<blink::mojom::AILanguageModel> language_model_remote;

  language_model_receivers_.Add(
      std::make_unique<UtilityAILanguageModel>(language_model,
                                               weak_ptr_factory_.GetWeakPtr()),
      language_model_remote.InitWithNewPipeAndPassReceiver());

  client->OnResult(
      std::move(language_model_remote),
      blink::mojom::AILanguageModelInstanceInfo::New(
          context_quota, context_usage,
          blink::mojom::AILanguageModelSamplingParams::New(),
          std::vector<blink::mojom::AILanguageModelPromptType>(
              enabled_input_types.begin(), enabled_input_types.end()),
          /*audio_sample_rate_hz=*/std::nullopt,
          /*audio_channel_count=*/std::nullopt));
}

void UtilityAIManager::CanCreateLanguageModel(
    blink::mojom::AILanguageModelCreateOptionsPtr options,
    CanCreateLanguageModelCallback callback) {
  v8::Global<v8::Object>& language_model_class = GetLanguageModelClass();
  {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope{isolate};
    v8::Global<v8::Object>& module_object =
        electron::api::local_ai_handler::GetModuleObject();
    if (!module_object.IsEmpty()) {
      gin_helper::EmitEvent(isolate, module_object.Get(isolate),
                            "-can-create-language-model");
    }
  }
  blink::mojom::ModelAvailabilityCheckResult availability =
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown;

  if (language_model_class.IsEmpty()) {
    std::move(callback).Run(availability);
  } else {
    // If a handler is set, we can create a language model.

    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope{isolate};

    v8::Local<v8::Value> val = gin_helper::CallMethod(
        isolate, language_model_class.Get(isolate), "availability", options);

    auto RunCallback = [](v8::Isolate* isolate,
                          CanCreateLanguageModelCallback callback,
                          v8::Local<v8::Value> result) {
      blink::mojom::ModelAvailabilityCheckResult availability =
          blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown;

      if (result->IsString() &&
          gin::ConvertFromV8(isolate, result, &availability)) {
        std::move(callback).Run(availability);
      } else {
        auto err = v8::Exception::TypeError(gin::StringToV8(
            isolate, "Invalid return value from LanguageModel.availability()"));
        node::errors::TriggerUncaughtException(isolate, err, {});
        std::move(callback).Run(
            blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
      }
    };

    if (val->IsPromise()) {
      auto promise = val.As<v8::Promise>();
      auto split_callback = base::SplitOnceCallback(std::move(callback));

      auto then_cb =
          base::BindOnce(RunCallback, isolate, std::move(split_callback.first));

      auto catch_cb = base::BindOnce(
          [](CanCreateLanguageModelCallback callback,
             v8::Local<v8::Value> result) {
            std::move(callback).Run(blink::mojom::ModelAvailabilityCheckResult::
                                        kUnavailableUnknown);
          },
          std::move(split_callback.second));

      std::ignore = promise->Then(
          isolate->GetCurrentContext(),
          gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
          gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
    } else {
      // The method is supposed to return a promise, but for
      // convenience allow developers to return a value directly
      RunCallback(isolate, std::move(callback), val);
    }
  }
}

void UtilityAIManager::CreateLanguageModel(
    mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
        client,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  v8::Global<v8::Object>& language_model_class = GetLanguageModelClass();

  // Can't create language model if there's no language model class
  if (language_model_class.IsEmpty()) {
    mojo::Remote<blink::mojom::AIManagerCreateLanguageModelClient>
        client_remote(std::move(client));
    client_remote->OnError(
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession,
        /*quota_error_info=*/nullptr);
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  gin_helper::Dictionary options_dict{
      isolate, gin::ConvertToV8(isolate, options).As<v8::Object>()};

  CreateLanguageModelInternal(isolate, std::move(client),
                              language_model_class.Get(isolate), "create",
                              std::move(options_dict), std::move(options));
}

void UtilityAIManager::CreateLanguageModelInternal(
    v8::Isolate* isolate,
    mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
        client,
    v8::Local<v8::Object> target,
    std::string_view method_name,
    gin_helper::Dictionary options_dict,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  DCHECK(method_name == "create" || method_name == "clone");

  std::string error_source = "LanguageModel." + std::string(method_name) + "()";

  mojo::RemoteSetElementId client_id =
      create_model_client_set_.Add(std::move(client));

  // Store the abort controller so the disconnect handler can abort it.
  v8::Local<v8::Object> abort_controller = util::CreateAbortController(isolate);

  abort_controllers_.emplace(client_id,
                             v8::Global<v8::Object>(isolate, abort_controller));

  options_dict.Set("signal", abort_controller
                                 ->Get(isolate->GetCurrentContext(),
                                       gin::StringToV8(isolate, "signal"))
                                 .ToLocalChecked());

  v8::Local<v8::Value> val =
      gin_helper::CallMethod(isolate, target, method_name.data(), options_dict);

  if (val->IsPromise()) {
    auto promise = val.As<v8::Promise>();

    auto then_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAIManager> weak_ptr, v8::Isolate* isolate,
           mojo::RemoteSetElementId client_id,
           blink::mojom::AILanguageModelCreateOptionsPtr options,
           std::string error_source, v8::Local<v8::Value> result) {
          if (weak_ptr) {
            if (result->IsObject() &&
                UtilityAILanguageModel::IsLanguageModel(isolate, result)) {
              weak_ptr->HandleLanguageModelResult(
                  isolate, result.As<v8::Object>(), client_id,
                  std::move(options));
            } else {
              auto err = v8::Exception::TypeError(gin::StringToV8(
                  isolate, "Invalid return value from " + error_source));
              node::errors::TriggerUncaughtException(isolate, err, {});
              weak_ptr->SendCreateLanguageModelError(
                  client_id, blink::mojom::AIManagerCreateClientError::
                                 kUnableToCreateSession);
            }
          }
        },
        weak_ptr_factory_.GetWeakPtr(), isolate, client_id, std::move(options),
        std::string(error_source));

    auto catch_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAIManager> weak_ptr,
           mojo::RemoteSetElementId client_id, v8::Local<v8::Value> result) {
          if (weak_ptr) {
            weak_ptr->SendCreateLanguageModelError(
                client_id, blink::mojom::AIManagerCreateClientError::
                               kUnableToCreateSession);
          }
        },
        weak_ptr_factory_.GetWeakPtr(), client_id);

    std::ignore = promise->Then(
        isolate->GetCurrentContext(),
        gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
        gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
  } else if (val->IsObject() &&
             UtilityAILanguageModel::IsLanguageModel(isolate, val)) {
    // The method is supposed to return a promise, but for
    // convenience allow developers to return a value directly
    HandleLanguageModelResult(isolate, val.As<v8::Object>(), client_id,
                              std::move(options));
  } else {
    auto err = v8::Exception::TypeError(gin::StringToV8(
        isolate, "Invalid return value from " + std::string(error_source)));
    node::errors::TriggerUncaughtException(isolate, err, {});
    SendCreateLanguageModelError(
        client_id,
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession);
  }
}

void UtilityAIManager::CanCreateSummarizer(
    blink::mojom::AISummarizerCreateOptionsPtr options,
    CanCreateSummarizerCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void UtilityAIManager::CreateSummarizer(
    mojo::PendingRemote<blink::mojom::AIManagerCreateSummarizerClient> client,
    blink::mojom::AISummarizerCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void UtilityAIManager::GetLanguageModelParams(
    GetLanguageModelParamsCallback callback) {
  NOTIMPLEMENTED();
}

void UtilityAIManager::CanCreateWriter(
    blink::mojom::AIWriterCreateOptionsPtr options,
    CanCreateWriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void UtilityAIManager::CreateWriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateWriterClient> client,
    blink::mojom::AIWriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void UtilityAIManager::CanCreateRewriter(
    blink::mojom::AIRewriterCreateOptionsPtr options,
    CanCreateRewriterCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void UtilityAIManager::CreateRewriter(
    mojo::PendingRemote<blink::mojom::AIManagerCreateRewriterClient> client,
    blink::mojom::AIRewriterCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void UtilityAIManager::CanCreateProofreader(
    blink::mojom::AIProofreaderCreateOptionsPtr options,
    CanCreateProofreaderCallback callback) {
  std::move(callback).Run(
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown);
}

void UtilityAIManager::CreateProofreader(
    mojo::PendingRemote<blink::mojom::AIManagerCreateProofreaderClient> client,
    blink::mojom::AIProofreaderCreateOptionsPtr options) {
  NOTIMPLEMENTED();
}

void UtilityAIManager::AddModelDownloadProgressObserver(
    mojo::PendingRemote<on_device_model::mojom::DownloadObserver>
        observer_remote) {
  NOTIMPLEMENTED();
}

}  // namespace electron
