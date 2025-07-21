// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/ai/utility_ai_manager.h"

#include <optional>
#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/notimplemented.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_util.h"
#include "shell/utility/ai/utility_ai_language_model.h"
#include "shell/utility/api/electron_api_local_ai_handler.h"
#include "third_party/blink/public/mojom/ai/ai_common.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
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
struct Converter<blink::mojom::AILanguageModelParamsPtr> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     blink::mojom::AILanguageModelParamsPtr* out) {
    gin_helper::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    auto params = blink::mojom::AILanguageModelParams::New();
    params->default_sampling_params =
        blink::mojom::AILanguageModelSamplingParams::New();
    params->max_sampling_params =
        blink::mojom::AILanguageModelSamplingParams::New();

    if (!dict.Get("defaultTopK", &(params->default_sampling_params->top_k)))
      return false;

    if (!dict.Get("defaultTemperature",
                  &(params->default_sampling_params->temperature)))
      return false;

    if (!dict.Get("maxTopK", &(params->max_sampling_params->top_k)))
      return false;

    if (!dict.Get("maxTemperature",
                  &(params->max_sampling_params->temperature)))
      return false;

    *out = std::move(params);
    return true;
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

    if (val->languages.has_value()) {
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

    // TODO - Need to include an AbortSignal in here
    auto dict = gin::Dictionary::CreateEmpty(isolate);

    if (!val->sampling_params.is_null()) {
      dict.Set("topK", val->sampling_params->top_k);
      dict.Set("temperature", val->sampling_params->temperature);
    }

    if (val->expected_inputs.has_value()) {
      dict.Set("expectedInputs", val->expected_inputs.value());
    }

    if (val->expected_outputs.has_value()) {
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
    : web_contents_id_(web_contents_id), security_origin_(security_origin) {}

UtilityAIManager::~UtilityAIManager() = default;

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
      details.Set("securityOrigin", security_origin_.GetURL().spec());

      // TODO - Add v8::TryCatch?
      v8::Local<v8::Value> val = handler->Run(details);

      // TODO - Can we validate that the class has the expected methods?
      if (!val->IsObject() ||
          !val->ToObject(isolate->GetCurrentContext())
               .ToLocalChecked()
               ->IsConstructor() ||
          !UtilityAILanguageModel::IsLanguageModelClass(isolate, val)) {
        isolate->ThrowException(v8::Exception::TypeError(
            gin::StringToV8(isolate, "Must provide a constructible class")));
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
  blink::mojom::AIManagerCreateLanguageModelClient* client =
      create_model_client_set_.Get(client_id);
  if (!client) {
    return;
  }

  client->OnError(error, /*quota_error_info=*/nullptr);
}

void UtilityAIManager::CreateLanguageModelInternal(
    v8::Isolate* isolate,
    v8::Local<v8::Object> language_model,
    mojo::RemoteSetElementId client_id,
    blink::mojom::AILanguageModelCreateOptionsPtr options) {
  mojo::PendingRemote<blink::mojom::AILanguageModel> language_model_remote;

  mojo::MakeSelfOwnedReceiver(
      std::make_unique<UtilityAILanguageModel>(language_model),
      language_model_remote.InitWithNewPipeAndPassReceiver());

  gin_helper::Dictionary dict;
  uint64_t input_usage = 0;
  uint64_t input_quota = 0;
  auto model_sampling_params =
      blink::mojom::AILanguageModelSamplingParams::New();

  if (!ConvertFromV8(isolate, language_model, &dict) ||
      !dict.Get("inputUsage", &input_usage) ||
      !dict.Get("inputQuota", &input_quota) ||
      !dict.Get("topK", &model_sampling_params->top_k) ||
      !dict.Get("temperature", &model_sampling_params->temperature)) {
    SendCreateLanguageModelError(
        client_id,
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession);
    return;
  }

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

  client->OnResult(
      std::move(language_model_remote),
      blink::mojom::AILanguageModelInstanceInfo::New(
          input_quota, input_usage, std::move(model_sampling_params),
          std::vector<blink::mojom::AILanguageModelPromptType>(
              enabled_input_types.begin(), enabled_input_types.end())));
}

void UtilityAIManager::CanCreateLanguageModel(
    blink::mojom::AILanguageModelCreateOptionsPtr options,
    CanCreateLanguageModelCallback callback) {
  v8::Global<v8::Object>& language_model_class = GetLanguageModelClass();
  blink::mojom::ModelAvailabilityCheckResult availability =
      blink::mojom::ModelAvailabilityCheckResult::kUnavailableUnknown;

  if (language_model_class.IsEmpty()) {
    std::move(callback).Run(availability);
  } else {
    // If a handler is set, we can create a language model.

    // TODO - Add v8::TryCatch?
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
        // TODO - Error is here
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
            // TODO - Error is here
            // TODO - An error here is killing the utility process
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

  mojo::RemoteSetElementId client_id =
      create_model_client_set_.Add(std::move(client));

  // Can't create language model if there's no language model class
  if (language_model_class.IsEmpty()) {
    SendCreateLanguageModelError(
        client_id,
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession);
    return;
  }

  // TODO - Add v8::TryCatch? Otherwise an error calling the method kills the
  // utility process
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  // TODO - Store off the abort controller somewhere we can use it in disconnect
  // handler
  v8::Local<v8::Object> abort_controller = util::CreateAbortController(isolate);

  gin_helper::Dictionary options_dict{
      isolate, gin::ConvertToV8(isolate, options).As<v8::Object>()};
  options_dict.Set("signal", abort_controller
                                 ->Get(isolate->GetCurrentContext(),
                                       gin::StringToV8(isolate, "signal"))
                                 .ToLocalChecked());

  v8::Local<v8::Value> val = gin_helper::CallMethod(
      isolate, language_model_class.Get(isolate), "create", options_dict);

  if (val->IsPromise()) {
    auto promise = val.As<v8::Promise>();

    auto then_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAIManager> weak_ptr, v8::Isolate* isolate,
           mojo::RemoteSetElementId client_id,
           blink::mojom::AILanguageModelCreateOptionsPtr options,
           v8::Local<v8::Value> result) {
          if (weak_ptr) {
            blink::mojom::AILanguageModelParamsPtr params;
            if (result->IsObject() &&
                UtilityAILanguageModel::IsLanguageModel(isolate, result)) {
              weak_ptr->CreateLanguageModelInternal(
                  isolate, result.As<v8::Object>(), client_id,
                  std::move(options));
            } else {
              // TODO - Error is here
              weak_ptr->SendCreateLanguageModelError(
                  client_id, blink::mojom::AIManagerCreateClientError::
                                 kUnableToCreateSession);
            }
          }
        },
        weak_ptr_factory_.GetWeakPtr(), isolate, client_id, std::move(options));

    auto catch_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAIManager> weak_ptr,
           mojo::RemoteSetElementId client_id, v8::Local<v8::Value> result) {
          // TODO - Error is here
          // TODO - Need to handle the promise rejection
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
    CreateLanguageModelInternal(isolate, val.As<v8::Object>(), client_id,
                                std::move(options));
    return;
  } else {
    // TODO - Error handling
    // TODO - Better error handling when the result is missing fields
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
  v8::Global<v8::Object>& language_model_class = GetLanguageModelClass();

  if (language_model_class.IsEmpty()) {
    std::move(callback).Run(nullptr);
  } else {
    // If a handler is set, we can get language model params

    // TODO - Add v8::TryCatch? Otherwise an error calling the method kills
    // the utility process
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope{isolate};

    v8::Local<v8::Value> val = gin_helper::CallMethod(
        isolate, language_model_class.Get(isolate), "params");

    auto RunCallback = [](v8::Isolate* isolate,
                          GetLanguageModelParamsCallback callback,
                          v8::Local<v8::Value> result) {
      blink::mojom::AILanguageModelParamsPtr params;

      if (result->IsObject() && gin::ConvertFromV8(isolate, result, &params)) {
        std::move(callback).Run(std::move(params));
        return;
      } else if (result->IsNull()) {
        std::move(callback).Run(nullptr);
      } else {
        // TODO - Error handling
        // TODO - Better error handling when the result is missing fields
        std::move(callback).Run(nullptr);
      }
    };

    if (val->IsPromise()) {
      auto promise = val.As<v8::Promise>();
      auto split_callback = base::SplitOnceCallback(std::move(callback));

      auto then_cb =
          base::BindOnce(RunCallback, isolate, std::move(split_callback.first));

      auto catch_cb = base::BindOnce(
          [](GetLanguageModelParamsCallback callback,
             v8::Local<v8::Value> result) {
            // TODO - Error is here
            // TODO - Need to handle the promise rejection
            std::move(callback).Run(nullptr);
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
    mojo::PendingRemote<blink::mojom::ModelDownloadProgressObserver>
        observer_remote) {
  NOTIMPLEMENTED();
}

}  // namespace electron
