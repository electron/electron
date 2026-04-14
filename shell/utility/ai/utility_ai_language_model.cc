// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/ai/utility_ai_language_model.h"

#include <string_view>

#include "base/no_destructor.h"
#include "base/notimplemented.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/event_emitter_caller.h"
#include "shell/common/node_includes.h"
#include "shell/common/node_util.h"
#include "shell/common/v8_util.h"
#include "shell/utility/ai/utility_ai_manager.h"
#include "third_party/blink/public/mojom/ai/ai_common.mojom.h"
#include "third_party/blink/public/mojom/ai/ai_language_model.mojom.h"
#include "third_party/blink/public/mojom/ai/model_streaming_responder.mojom.h"

namespace gin {

template <>
struct Converter<on_device_model::mojom::ResponseConstraintPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const on_device_model::mojom::ResponseConstraintPtr& val) {
    if (val.is_null())
      return v8::Undefined(isolate);

    if (val->is_json_schema()) {
      return v8::JSON::Parse(isolate->GetCurrentContext(),
                             StringToV8(isolate, val->get_json_schema()))
          .ToLocalChecked();
    } else if (val->is_regex()) {
      return v8::RegExp::New(isolate->GetCurrentContext(),
                             StringToV8(isolate, val->get_regex()),
                             v8::RegExp::kNone)
          .ToLocalChecked();
    }

    return v8::Undefined(isolate);
  }
};

template <>
struct Converter<blink::mojom::AILanguageModelPromptRole> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      blink::mojom::AILanguageModelPromptRole value) {
    switch (value) {
      case blink::mojom::AILanguageModelPromptRole::kSystem:
        return StringToV8(isolate, "system");
      case blink::mojom::AILanguageModelPromptRole::kUser:
        return StringToV8(isolate, "user");
      case blink::mojom::AILanguageModelPromptRole::kAssistant:
        return StringToV8(isolate, "assistant");
      default:
        return StringToV8(isolate, "unknown");
    }
  }
};

template <>
struct Converter<blink::mojom::AILanguageModelPromptContentPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const blink::mojom::AILanguageModelPromptContentPtr& val) {
    if (val.is_null())
      return v8::Undefined(isolate);

    auto dict = gin::Dictionary::CreateEmpty(isolate);

    if (val->is_text()) {
      dict.Set("type", "text");
      dict.Set("value", val->get_text());
    } else if (val->is_bitmap()) {
      // Convert the bitmap to an ArrayBuffer
      // TODO - Are we going to make any guarantees about the shape of the image
      // data?
      SkBitmap& bitmap = val->get_bitmap();

      const auto dst_info = SkImageInfo::MakeN32Premul(bitmap.dimensions());
      const size_t dst_n_bytes = dst_info.computeMinByteSize();
      auto dst_buf = v8::ArrayBuffer::New(isolate, dst_n_bytes);

      if (!bitmap.readPixels(dst_info, dst_buf->Data(), dst_info.minRowBytes(),
                             0, 0)) {
        auto err = v8::Exception::TypeError(
            gin::StringToV8(isolate, "Invalid bitmap content in prompt"));
        node::errors::TriggerUncaughtException(isolate, err, {});
      }

      dict.Set("type", "image");
      dict.Set("value", dst_buf);
    } else if (val->is_audio()) {
      // Convert the audio data to an ArrayBuffer
      // TODO - Are we going to make any guarantees about the shape of the audio
      // data?
      on_device_model::mojom::AudioDataPtr& audio_data = val->get_audio();
      std::vector<float>& raw_data = audio_data->data;

      const size_t dst_n_bytes =
          sizeof(std::remove_reference_t<decltype(raw_data)>::value_type) *
          raw_data.size();
      auto dst_buf = v8::ArrayBuffer::New(isolate, dst_n_bytes);

      UNSAFE_BUFFERS(
          std::ranges::copy(raw_data, static_cast<char*>(dst_buf->Data())));

      dict.Set("type", "audio");
      dict.Set("value", dst_buf);
    }

    return ConvertToV8(isolate, dict);
  }
};

v8::Local<v8::Value> Converter<blink::mojom::AILanguageModelPromptPtr>::ToV8(
    v8::Isolate* isolate,
    const blink::mojom::AILanguageModelPromptPtr& val) {
  if (val.is_null())
    return v8::Undefined(isolate);

  auto dict = gin::Dictionary::CreateEmpty(isolate);

  dict.Set("role", val->role);
  dict.Set("content", val->content);
  dict.Set("prefix", val->is_prefix);

  return ConvertToV8(isolate, dict);
}

}  // namespace gin

namespace electron {

namespace {

constexpr std::string_view kIsReadableStreamKey = "isReadableStream";
constexpr std::string_view kIsLanguageModelKey = "isLanguageModel";
constexpr std::string_view kIsLanguageModelClassKey = "isLanguageModelClass";

v8::Local<v8::Function> GetPrivateBoolean(v8::Isolate* const isolate,
                                          const v8::Local<v8::Context>& context,
                                          std::string_view key) {
  auto binding_key = gin::StringToV8(isolate, key);
  auto private_binding_key = v8::Private::ForApi(isolate, binding_key);
  auto global_object = context->Global();
  auto value =
      global_object->GetPrivate(context, private_binding_key).ToLocalChecked();
  if (value.IsEmpty() || !value->IsFunction()) {
    LOG(FATAL) << "Attempted to get the '" << key
               << "' value but it was missing";
  }
  return value.As<v8::Function>();
}

bool IsReadableStream(v8::Isolate* isolate, v8::Local<v8::Value> val) {
  static base::NoDestructor<v8::Global<v8::Function>> is_readable_stream;

  auto context = isolate->GetCurrentContext();

  if (is_readable_stream.get()->IsEmpty()) {
    is_readable_stream->Reset(
        isolate, GetPrivateBoolean(isolate, context, kIsReadableStreamKey));
  }

  v8::Local<v8::Value> args[] = {val};
  v8::Local<v8::Value> result =
      is_readable_stream->Get(isolate)
          ->Call(context, v8::Null(isolate), std::size(args), args)
          .ToLocalChecked();

  return result->IsBoolean() && result.As<v8::Boolean>()->Value();
}

uint64_t GetContextUsage(v8::Isolate* isolate,
                         v8::Local<v8::Object> language_model) {
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::Value> val =
      language_model->Get(context, gin::StringToV8(isolate, "contextUsage"))
          .ToLocalChecked();
  uint64_t token_count = 0;
  if (val->IsNumber()) {
    gin::ConvertFromV8(isolate, val, &token_count);
  }
  return token_count;
}

// Owns itself. Will live as long as there's more data to process
// and the Mojo remote is still connected.
class PromptResponder {
 public:
  PromptResponder(v8::Isolate* isolate,
                  v8::Local<v8::Value> value,
                  v8::Local<v8::Object> abort_controller,
                  v8::Local<v8::Object> language_model,
                  mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
                      pending_responder,
                  UtilityAILanguageModel* model) {
    abort_controller_.Reset(isolate, abort_controller);
    language_model_.Reset(isolate, language_model);
    responder_.Bind(std::move(pending_responder));
    responder_.set_disconnect_handler(base::BindOnce(
        &PromptResponder::DeleteThis, weak_ptr_factory_.GetWeakPtr()));

    destroy_subscription_ = model->AddDestroyObserver(base::BindRepeating(
        &PromptResponder::OnModelDestroyed, weak_ptr_factory_.GetWeakPtr()));

    Respond(isolate, value);
  }

  // disable copy
  PromptResponder(const PromptResponder&) = delete;
  PromptResponder& operator=(const PromptResponder&) = delete;

 private:
  void OnModelDestroyed() {
    // Drop the subscription since the model is already being destroyed.
    destroy_subscription_ = {};
    responder_->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorSessionDestroyed,
        /*quota_error_info=*/nullptr);
    DeleteThis();
  }

  void Respond(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    if (value->IsPromise()) {
      auto promise = value.As<v8::Promise>();

      auto then_cb = base::BindOnce(
          [](base::WeakPtr<PromptResponder> weak_ptr, v8::Isolate* isolate,
             v8::Local<v8::Value> result) {
            if (weak_ptr) {
              weak_ptr->RespondImplementation(isolate, result);
            }
          },
          weak_ptr_factory_.GetWeakPtr(), isolate);

      auto catch_cb = base::BindOnce(
          [](base::WeakPtr<PromptResponder> weak_ptr,
             v8::Local<v8::Value> result) {
            if (weak_ptr) {
              weak_ptr->SendError();
              weak_ptr->DeleteThis();
            }
          },
          weak_ptr_factory_.GetWeakPtr());

      std::ignore = promise->Then(
          isolate->GetCurrentContext(),
          gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
          gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
    } else {
      RespondImplementation(isolate, value);
    }
  }

  void RespondImplementation(v8::Isolate* isolate, v8::Local<v8::Value> val) {
    std::string response;

    if (val->IsString() && gin::ConvertFromV8(isolate, val, &response)) {
      responder_->OnStreaming(response);
      uint64_t token_count =
          GetContextUsage(isolate, language_model_.Get(isolate));
      responder_->OnCompletion(
          blink::mojom::ModelExecutionContextInfo::New(token_count));
      completed_ = true;
      DeleteThis();
    } else if (IsReadableStream(isolate, val)) {
      v8::Local<v8::Value> reader =
          gin_helper::CallMethod(isolate, val.As<v8::Object>(), "getReader");
      DCHECK(reader->IsObject());
      readable_stream_reader_.Reset(isolate, reader.As<v8::Object>());
      Read(isolate);
    } else {
      SendError();
      DeleteThis();
      auto err = v8::Exception::TypeError(gin::StringToV8(
          isolate, "Invalid return value from LanguageModel.prompt()"));
      node::errors::TriggerUncaughtException(isolate, err, {});
    }
  }

  void Read(v8::Isolate* isolate) {
    v8::Local<v8::Value> val = gin_helper::CallMethod(
        isolate, readable_stream_reader_.Get(isolate), "read");
    DCHECK(val->IsPromise());

    auto promise = val.As<v8::Promise>();

    auto then_cb = base::BindOnce(
        [](base::WeakPtr<PromptResponder> weak_ptr, v8::Isolate* isolate,
           v8::Local<v8::Value> result) {
          if (weak_ptr) {
            CHECK(result->IsObject());

            v8::Local<v8::Value> done =
                result.As<v8::Object>()
                    ->Get(isolate->GetCurrentContext(),
                          gin::StringToV8(isolate, "done"))
                    .ToLocalChecked();
            CHECK(done->IsBoolean());

            if (done.As<v8::Boolean>()->Value()) {
              uint64_t token_count = GetContextUsage(
                  isolate, weak_ptr->language_model_.Get(isolate));
              weak_ptr->responder_->OnCompletion(
                  blink::mojom::ModelExecutionContextInfo::New(token_count));
              weak_ptr->completed_ = true;
              weak_ptr->DeleteThis();
            } else {
              v8::Local<v8::Value> val =
                  result.As<v8::Object>()
                      ->Get(isolate->GetCurrentContext(),
                            gin::StringToV8(isolate, "value"))
                      .ToLocalChecked();
              DCHECK(val->IsString());

              std::string value;

              if (gin::ConvertFromV8(isolate, val, &value)) {
                weak_ptr->responder_->OnStreaming(value);
                weak_ptr->Read(isolate);
              } else {
                weak_ptr->SendError();
                weak_ptr->DeleteThis();
              }
            }
          }
        },
        weak_ptr_factory_.GetWeakPtr(), isolate);

    auto catch_cb = base::BindOnce(
        [](base::WeakPtr<PromptResponder> weak_ptr,
           v8::Local<v8::Value> result) {
          if (weak_ptr) {
            weak_ptr->SendError();
            weak_ptr->DeleteThis();
          }
        },
        weak_ptr_factory_.GetWeakPtr());

    std::ignore = promise->Then(
        isolate->GetCurrentContext(),
        gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
        gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
  }

  void SendError() {
    responder_->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorUnknown,
        /*quota_error_info=*/nullptr);
  }

  void DeleteThis() {
    destroy_subscription_ = {};
    weak_ptr_factory_.InvalidateWeakPtrs();

    if (!completed_) {
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      v8::HandleScope scope{isolate};

      if (!readable_stream_reader_.IsEmpty()) {
        gin_helper::CallMethod(isolate, readable_stream_reader_.Get(isolate),
                               "cancel");
      }

      gin_helper::CallMethod(isolate, abort_controller_.Get(isolate), "abort");
    }

    delete this;
  }

  bool completed_ = false;
  v8::Global<v8::Object> readable_stream_reader_;
  v8::Global<v8::Object> abort_controller_;
  v8::Global<v8::Object> language_model_;
  mojo::Remote<blink::mojom::ModelStreamingResponder> responder_;
  base::CallbackListSubscription destroy_subscription_;

  base::WeakPtrFactory<PromptResponder> weak_ptr_factory_{this};
};

}  // namespace

UtilityAILanguageModel::UtilityAILanguageModel(
    v8::Local<v8::Object> language_model,
    base::WeakPtr<UtilityAIManager> manager)
    : manager_(std::move(manager)) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  language_model_.Reset(isolate, language_model);
  responder_set_.set_disconnect_handler(
      base::BindRepeating(&UtilityAILanguageModel::OnResponderDisconnect,
                          weak_ptr_factory_.GetWeakPtr()));
}

UtilityAILanguageModel::~UtilityAILanguageModel() {
  if (!is_destroyed_) {
    Destroy();
  }
}

base::CallbackListSubscription UtilityAILanguageModel::AddDestroyObserver(
    base::RepeatingClosure callback) {
  return on_destroy_.Add(std::move(callback));
}

void UtilityAILanguageModel::OnResponderDisconnect(
    mojo::RemoteSetElementId id) {
  auto it = abort_controllers_.find(id);
  if (it != abort_controllers_.end()) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope scope{isolate};
    gin_helper::CallMethod(isolate, it->second.Get(isolate), "abort");
    abort_controllers_.erase(it);
  }
}

blink::mojom::ModelStreamingResponder* UtilityAILanguageModel::GetResponder(
    mojo::RemoteSetElementId responder_id) {
  return responder_set_.Get(responder_id);
}

// static
bool UtilityAILanguageModel::IsLanguageModel(v8::Isolate* isolate,
                                             v8::Local<v8::Value> val) {
  static base::NoDestructor<v8::Global<v8::Function>> is_language_model;

  auto context = isolate->GetCurrentContext();

  if (is_language_model.get()->IsEmpty()) {
    is_language_model->Reset(
        isolate, GetPrivateBoolean(isolate, context, kIsLanguageModelKey));
  }

  v8::Local<v8::Value> args[] = {val};
  v8::Local<v8::Value> result =
      is_language_model->Get(isolate)
          ->Call(context, v8::Null(isolate), std::size(args), args)
          .ToLocalChecked();

  return result->IsBoolean() && result.As<v8::Boolean>()->Value();
}

// static
bool UtilityAILanguageModel::IsLanguageModelClass(v8::Isolate* isolate,
                                                  v8::Local<v8::Value> val) {
  static base::NoDestructor<v8::Global<v8::Function>> is_language_model_class;

  auto context = isolate->GetCurrentContext();

  if (is_language_model_class.get()->IsEmpty()) {
    is_language_model_class->Reset(
        isolate, GetPrivateBoolean(isolate, context, kIsLanguageModelClassKey));
  }

  v8::Local<v8::Value> args[] = {val};
  v8::Local<v8::Value> result =
      is_language_model_class->Get(isolate)
          ->Call(context, v8::Null(isolate), std::size(args), args)
          .ToLocalChecked();

  return result->IsBoolean() && result.As<v8::Boolean>()->Value();
}

void UtilityAILanguageModel::Prompt(
    std::vector<blink::mojom::AILanguageModelPromptPtr> prompts,
    on_device_model::mojom::ResponseConstraintPtr constraint,
    mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
        pending_responder) {
  if (is_destroyed_) {
    mojo::Remote<blink::mojom::ModelStreamingResponder> responder(
        std::move(pending_responder));
    responder->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorSessionDestroyed,
        /*quota_error_info=*/nullptr);
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  v8::Local<v8::Object> abort_controller = util::CreateAbortController(isolate);

  auto options = gin_helper::Dictionary::CreateEmpty(isolate);
  if (!constraint.is_null()) {
    options.Set("responseConstraint", gin::ConvertToV8(isolate, constraint));
  }
  options.Set("signal", abort_controller
                            ->Get(isolate->GetCurrentContext(),
                                  gin::StringToV8(isolate, "signal"))
                            .ToLocalChecked());

  v8::Local<v8::Value> val = gin_helper::CallMethod(
      isolate, language_model_.Get(isolate), "prompt", prompts, options);

  new PromptResponder(isolate, val, abort_controller,
                      language_model_.Get(isolate),
                      std::move(pending_responder), this);
}

void UtilityAILanguageModel::Append(
    std::vector<blink::mojom::AILanguageModelPromptPtr> prompts,
    mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
        pending_responder) {
  if (is_destroyed_) {
    mojo::Remote<blink::mojom::ModelStreamingResponder> responder(
        std::move(pending_responder));
    responder->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorSessionDestroyed,
        /*quota_error_info=*/nullptr);
    return;
  }

  mojo::RemoteSetElementId responder_id =
      responder_set_.Add(std::move(pending_responder));

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  v8::Local<v8::Object> abort_controller = util::CreateAbortController(isolate);
  abort_controllers_.emplace(responder_id,
                             v8::Global<v8::Object>(isolate, abort_controller));

  auto options = gin_helper::Dictionary::CreateEmpty(isolate);
  options.Set("signal", abort_controller
                            ->Get(isolate->GetCurrentContext(),
                                  gin::StringToV8(isolate, "signal"))
                            .ToLocalChecked());

  v8::Local<v8::Value> val = gin_helper::CallMethod(
      isolate, language_model_.Get(isolate), "append", prompts, options);

  auto SendResponse =
      [](base::WeakPtr<UtilityAILanguageModel> weak_ptr, v8::Isolate* isolate,
         mojo::RemoteSetElementId responder_id, v8::Local<v8::Value> result) {
        if (!weak_ptr)
          return;
        weak_ptr->abort_controllers_.erase(responder_id);

        blink::mojom::ModelStreamingResponder* responder =
            weak_ptr->GetResponder(responder_id);
        if (!responder) {
          return;
        }

        uint64_t token_count =
            GetContextUsage(isolate, weak_ptr->language_model_.Get(isolate));
        responder->OnCompletion(
            blink::mojom::ModelExecutionContextInfo::New(token_count));
      };

  if (val->IsPromise()) {
    auto promise = val.As<v8::Promise>();

    auto then_cb = base::BindOnce(SendResponse, weak_ptr_factory_.GetWeakPtr(),
                                  isolate, responder_id);

    auto catch_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAILanguageModel> weak_ptr,
           mojo::RemoteSetElementId responder_id, v8::Local<v8::Value> result) {
          if (!weak_ptr)
            return;
          weak_ptr->abort_controllers_.erase(responder_id);

          blink::mojom::ModelStreamingResponder* responder =
              weak_ptr->GetResponder(responder_id);
          if (!responder) {
            return;
          }

          responder->OnError(
              blink::mojom::ModelStreamingResponseStatus::kErrorUnknown,
              /*quota_error_info=*/nullptr);
        },
        weak_ptr_factory_.GetWeakPtr(), responder_id);

    std::ignore = promise->Then(
        isolate->GetCurrentContext(),
        gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
        gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
  } else {
    // The method is supposed to return a promise, but for
    // convenience allow developers to return a value directly
    SendResponse(weak_ptr_factory_.GetWeakPtr(), isolate, responder_id, val);
  }
}

void UtilityAILanguageModel::Fork(
    mojo::PendingRemote<blink::mojom::AIManagerCreateLanguageModelClient>
        client) {
  if (is_destroyed_ || !manager_) {
    mojo::Remote<blink::mojom::AIManagerCreateLanguageModelClient>
        client_remote(std::move(client));
    client_remote->OnError(
        blink::mojom::AIManagerCreateClientError::kUnableToCreateSession,
        /*quota_error_info=*/nullptr);
    return;
  }

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  manager_->CreateLanguageModelInternal(
      isolate, std::move(client), language_model_.Get(isolate), "clone",
      gin_helper::Dictionary::CreateEmpty(isolate),
      blink::mojom::AILanguageModelCreateOptions::New());
}

void UtilityAILanguageModel::Destroy() {
  if (is_destroyed_) {
    return;
  }

  is_destroyed_ = true;

  // Notify observers (e.g. in-progress PromptResponders) before
  // tearing down the responder set and abort controllers.
  on_destroy_.Notify();

  for (auto& responder : responder_set_) {
    responder->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorSessionDestroyed,
        /*quota_error_info=*/nullptr);
  }
  responder_set_.Clear();

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  for (auto& [id, controller] : abort_controllers_) {
    gin_helper::CallMethod(isolate, controller.Get(isolate), "abort");
  }
  abort_controllers_.clear();

  for (auto& controller : measure_abort_controllers_) {
    gin_helper::CallMethod(isolate, controller.Get(isolate), "abort");
  }
  measure_abort_controllers_.clear();

  gin_helper::CallMethod(isolate, language_model_.Get(isolate), "destroy");
}

void UtilityAILanguageModel::MeasureInputUsage(
    std::vector<blink::mojom::AILanguageModelPromptPtr> input,
    MeasureInputUsageCallback callback) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope{isolate};

  v8::Local<v8::Object> abort_controller = util::CreateAbortController(isolate);
  measure_abort_controllers_.emplace_back(isolate, abort_controller);
  auto abort_it = std::prev(measure_abort_controllers_.end());

  auto options = gin_helper::Dictionary::CreateEmpty(isolate);
  options.Set("signal", abort_controller
                            ->Get(isolate->GetCurrentContext(),
                                  gin::StringToV8(isolate, "signal"))
                            .ToLocalChecked());

  v8::Local<v8::Value> val =
      gin_helper::CallMethod(isolate, language_model_.Get(isolate),
                             "measureContextUsage", input, options);

  auto RunCallback = [](base::WeakPtr<UtilityAILanguageModel> weak_ptr,
                        std::list<v8::Global<v8::Object>>::iterator abort_it,
                        v8::Isolate* isolate,
                        MeasureInputUsageCallback callback,
                        v8::Local<v8::Value> result) {
    if (weak_ptr) {
      weak_ptr->measure_abort_controllers_.erase(abort_it);
    }

    uint32_t input_tokens = 0;

    if (result->IsNumber() &&
        gin::ConvertFromV8(isolate, result, &input_tokens)) {
      std::move(callback).Run(std::move(input_tokens));
    } else if (result->IsNull()) {
      std::move(callback).Run(std::nullopt);
    } else {
      std::move(callback).Run(std::nullopt);
      auto err = v8::Exception::TypeError(gin::StringToV8(
          isolate,
          "Invalid return value from LanguageModel.measureContextUsage()"));
      node::errors::TriggerUncaughtException(isolate, err, {});
    }
  };

  if (val->IsPromise()) {
    auto promise = val.As<v8::Promise>();
    auto split_callback = base::SplitOnceCallback(std::move(callback));

    auto then_cb =
        base::BindOnce(RunCallback, weak_ptr_factory_.GetWeakPtr(), abort_it,
                       isolate, std::move(split_callback.first));

    auto catch_cb = base::BindOnce(
        [](base::WeakPtr<UtilityAILanguageModel> weak_ptr,
           std::list<v8::Global<v8::Object>>::iterator abort_it,
           MeasureInputUsageCallback callback, v8::Local<v8::Value> result) {
          if (weak_ptr) {
            weak_ptr->measure_abort_controllers_.erase(abort_it);
          }
          std::move(callback).Run(std::nullopt);
        },
        weak_ptr_factory_.GetWeakPtr(), abort_it,
        std::move(split_callback.second));

    std::ignore = promise->Then(
        isolate->GetCurrentContext(),
        gin::ConvertToV8(isolate, std::move(then_cb)).As<v8::Function>(),
        gin::ConvertToV8(isolate, std::move(catch_cb)).As<v8::Function>());
  } else {
    // The method is supposed to return a promise, but for
    // convenience allow developers to return a value directly
    RunCallback(weak_ptr_factory_.GetWeakPtr(), abort_it, isolate,
                std::move(callback), val);
  }
}

}  // namespace electron
