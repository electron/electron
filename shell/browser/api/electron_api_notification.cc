// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_notification.h"

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/uuid.h"
#include "build/build_config.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/handle.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/notifications/win/windows_toast_activator.h"
#endif

namespace gin {

template <>
struct Converter<electron::NotificationAction> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::NotificationAction* out) {
    gin::Dictionary dict(isolate);
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    if (!dict.Get("type", &(out->type))) {
      return false;
    }
    dict.Get("text", &(out->text));
    std::vector<std::u16string> items;
    if (dict.Get("items", &items))
      out->items = std::move(items);
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::NotificationAction val) {
    auto dict = gin::Dictionary::CreateEmpty(isolate);
    dict.Set("text", val.text);
    dict.Set("type", val.type);
    if (!val.items.empty()) {
      dict.Set("items", val.items);
    }
    return ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

#if BUILDFLAG(IS_WIN)
namespace {

// Windows appends id/groupId into the toast launch string as key=value pairs;
// reject delimiter and control bytes so values cannot spoof extra pairs.
bool NotificationIdOrGroupContainsInvalidWindowsToastChars(
    const std::string& s) {
  for (unsigned char c : s) {
    if (c == '&' || c == '=')
      return true;
    if (c < 0x20 || c == 0x7f)
      return true;
  }
  return false;
}

}  // namespace
#endif

namespace electron::api {

gin::DeprecatedWrapperInfo Notification::kWrapperInfo = {
    gin::kEmbedderNativeGin};

Notification::Notification(gin::Arguments* args) {
  presenter_ = static_cast<ElectronBrowserClient*>(ElectronBrowserClient::Get())
                   ->GetNotificationPresenter();

  gin::Dictionary opts(nullptr);
  if (args->GetNext(&opts)) {
    opts.Get("id", &id_);
    opts.Get("groupId", &group_id_);
    opts.Get("groupTitle", &group_title_);
    opts.Get("title", &title_);
    opts.Get("subtitle", &subtitle_);
    opts.Get("body", &body_);
    opts.Get("icon", &icon_);
    opts.Get("silent", &silent_);
    opts.Get("replyPlaceholder", &reply_placeholder_);
    opts.Get("urgency", &urgency_);
    opts.Get("hasReply", &has_reply_);
    opts.Get("timeoutType", &timeout_type_);
    opts.Get("actions", &actions_);
    opts.Get("sound", &sound_);
    opts.Get("closeButtonText", &close_button_text_);
    opts.Get("toastXml", &toast_xml_);
  }

  if (id_.empty())
    id_ = base::Uuid::GenerateRandomV4().AsLowercaseString();
}

Notification::Notification(const NotificationInfo& info)
    : id_(info.id),
      raw_group_id_(info.group_id),
      title_(base::UTF8ToUTF16(info.title)),
      subtitle_(base::UTF8ToUTF16(info.subtitle)),
      body_(base::UTF8ToUTF16(info.body)),
      is_restored_(true),
      presenter_(nullptr) {
#if BUILDFLAG(IS_WIN)
  // Filter out the Windows default group ("Notifications") from the
  // JS-visible groupId — it's an internal implementation detail.
  if (raw_group_id_ != "Notifications")
    group_id_ = raw_group_id_;
#else
  group_id_ = raw_group_id_;
#endif
}

Notification::~Notification() {
  if (notification_) {
    notification_->set_delegate(nullptr);
    // For restored notifications, destroy the platform notification to remove
    // it from the presenter's set. The platform-level is_restored_ flag ensures
    // this won't remove the notification from Notification Center.
    // For normal notifications, Close() is called before destruction which
    // already cleans up, so notification_ will be null here.
    if (is_restored_)
      notification_->Destroy();
  }
}

// static
gin_helper::Handle<Notification> Notification::New(
    gin_helper::ErrorThrower thrower,
    gin::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    thrower.ThrowError("Cannot create Notification before app is ready");
    return {};
  }

  auto handle =
      gin_helper::CreateHandle(thrower.isolate(), new Notification(args));

#if BUILDFLAG(IS_WIN)
  constexpr size_t kMaxTagLength = 64;
  auto* notif = handle.get();
  if (NotificationIdOrGroupContainsInvalidWindowsToastChars(notif->id_)) {
    thrower.ThrowError(
        "Notification id cannot contain '&', '=', or control characters");
    return {};
  }
  if (NotificationIdOrGroupContainsInvalidWindowsToastChars(notif->group_id_)) {
    thrower.ThrowError(
        "Notification groupId cannot contain '&', '=', or control "
        "characters");
    return {};
  }
  if (!notif->id_.empty() &&
      base::UTF8ToWide(notif->id_).length() > kMaxTagLength) {
    thrower.ThrowError(
        "Notification id exceeds Windows limit of 64 UTF-16 characters");
    return {};
  }
  if (!notif->group_id_.empty() &&
      base::UTF8ToWide(notif->group_id_).length() > kMaxTagLength) {
    thrower.ThrowError(
        "Notification groupId exceeds Windows limit of 64 UTF-16 characters");
    return {};
  }
  if (!notif->group_title_.empty() && notif->group_id_.empty()) {
    thrower.ThrowError("Notification groupTitle requires groupId to be set");
    return {};
  }
#endif

  return handle;
}

// Setters
void Notification::SetTitle(const std::u16string& new_title) {
  title_ = new_title;
}

void Notification::SetSubtitle(const std::u16string& new_subtitle) {
  subtitle_ = new_subtitle;
}

void Notification::SetBody(const std::u16string& new_body) {
  body_ = new_body;
}

void Notification::SetSilent(bool new_silent) {
  silent_ = new_silent;
}

void Notification::SetHasReply(bool new_has_reply) {
  has_reply_ = new_has_reply;
}

void Notification::SetTimeoutType(const std::u16string& new_timeout_type) {
  timeout_type_ = new_timeout_type;
}

void Notification::SetReplyPlaceholder(const std::u16string& new_placeholder) {
  reply_placeholder_ = new_placeholder;
}

void Notification::SetSound(const std::u16string& new_sound) {
  sound_ = new_sound;
}

void Notification::SetUrgency(const std::u16string& new_urgency) {
  urgency_ = new_urgency;
}

void Notification::SetActions(
    const std::vector<electron::NotificationAction>& actions) {
  actions_ = actions;
}

void Notification::SetCloseButtonText(const std::u16string& text) {
  close_button_text_ = text;
}

void Notification::SetToastXml(const std::u16string& new_toast_xml) {
  toast_xml_ = new_toast_xml;
}

void Notification::NotificationAction(int action_index, int selection_index) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  gin_helper::internal::Event* event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      event->GetWrapper(isolate).ToLocalChecked();

  gin_helper::Dictionary dict(isolate, event_object);
  dict.Set("selectionIndex", selection_index);
  dict.Set("actionIndex", action_index);

  EmitWithoutEvent("action", event_object, action_index, selection_index);
}

void Notification::NotificationClick() {
  Emit("click");
}

void Notification::NotificationReplied(const std::string& reply) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);

  gin_helper::internal::Event* event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object =
      event->GetWrapper(isolate).ToLocalChecked();

  gin_helper::Dictionary dict(isolate, event_object);
  dict.Set("reply", reply);

  EmitWithoutEvent("reply", event_object, reply);
}

void Notification::NotificationDisplayed() {
  Emit("show");
}

void Notification::NotificationFailed(const std::string& error) {
  Emit("failed", error);
}

void Notification::NotificationDestroyed() {}

void Notification::NotificationClosed(const std::string& reason) {
  if (reason.empty()) {
    Emit("close");
  } else {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);

    gin_helper::internal::Event* event =
        gin_helper::internal::Event::New(isolate);
    v8::Local<v8::Object> event_object =
        event->GetWrapper(isolate).ToLocalChecked();

    gin_helper::Dictionary dict(isolate, event_object);
    dict.Set("reason", reason);

    EmitWithoutEvent("close", event_object);
  }
}

void Notification::Close() {
  if (notification_) {
    if (notification_->is_dismissed()) {
      notification_->Remove();
    } else {
      notification_->Dismiss();
    }
    notification_->set_delegate(nullptr);
    notification_.reset();
  }
}

// Showing notifications
void Notification::Show() {
  // Restored notifications are read-only snapshots from Notification Center.
  // Re-showing them would remove the original and create a duplicate.
  if (is_restored_)
    return;

  Close();
  if (presenter_) {
    notification_ = presenter_->CreateNotification(this, id_);
    if (notification_) {
      electron::NotificationOptions options;
      options.title = title_;
      options.subtitle = subtitle_;
      options.msg = body_;
      options.icon_url = GURL();
      options.icon = icon_.AsBitmap();
      options.silent = silent_;
      options.has_reply = has_reply_;
      options.timeout_type = timeout_type_;
      options.reply_placeholder = reply_placeholder_;
      options.actions = actions_;
      options.sound = sound_;
      options.close_button_text = close_button_text_;
      options.urgency = urgency_;
      options.toast_xml = toast_xml_;
      options.group_id = group_id_;
      options.group_title = group_title_;
      notification_->Show(options);
    }
  }
}

bool Notification::IsSupported() {
  return !!static_cast<ElectronBrowserClient*>(ElectronBrowserClient::Get())
               ->GetNotificationPresenter();
}

#if BUILDFLAG(IS_WIN)
namespace {

// Helper to convert ActivationArguments to JS object
v8::Local<v8::Value> ActivationArgumentsToV8(
    v8::Isolate* isolate,
    const electron::ActivationArguments& details) {
  gin_helper::Dictionary dict = gin_helper::Dictionary::CreateEmpty(isolate);
  dict.Set("type", details.type);
  dict.Set("arguments", details.arguments);

  if (details.type == "action") {
    dict.Set("actionIndex", details.action_index);
  } else if (details.type == "reply") {
    dict.Set("reply", details.reply);
  }

  if (!details.user_inputs.empty()) {
    gin_helper::Dictionary inputs =
        gin_helper::Dictionary::CreateEmpty(isolate);
    for (const auto& [key, value] : details.user_inputs) {
      inputs.Set(key, value);
    }
    dict.Set("userInputs", inputs);
  }

  return dict.GetHandle();
}

// Storage for the JavaScript callback (persistent so it survives GC).
// Uses base::NoDestructor to avoid exit-time destructor issues with globals.
// v8::Global supports Reset() for reassignment.
base::NoDestructor<v8::Global<v8::Function>> g_js_launch_callback;

void InvokeJsCallback(const electron::ActivationArguments& details) {
  if (g_js_launch_callback->IsEmpty())
    return;

  v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  v8::Local<v8::Function> callback = g_js_launch_callback->Get(isolate);
  v8::Local<v8::Value> argv[] = {ActivationArgumentsToV8(isolate, details)};

  v8::TryCatch try_catch(isolate);
  callback->Call(context, v8::Undefined(isolate), 1, argv)
      .FromMaybe(v8::Local<v8::Value>());
  // Callback stays registered for future activations
}

}  // namespace

// static
void Notification::HandleActivation(v8::Isolate* isolate,
                                    v8::Local<v8::Function> callback) {
  // Replace any previous callback using Reset (v8::Global supports this)
  g_js_launch_callback->Reset(isolate, callback);

  // Register the C++ callback that invokes the JS callback.
  // - If activation details already exist, callback is invoked immediately.
  // - Callback remains registered for all future activations.
  electron::SetActivationHandler(
      [](const electron::ActivationArguments& details) {
        InvokeJsCallback(details);
      });
}
#endif

// static
v8::Local<v8::Promise> Notification::GetHistory(v8::Isolate* isolate) {
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* presenter =
      static_cast<ElectronBrowserClient*>(ElectronBrowserClient::Get())
          ->GetNotificationPresenter();
  if (!presenter) {
    promise.Resolve(v8::Array::New(isolate));
    return handle;
  }

  presenter->GetDeliveredNotifications(base::BindOnce(
      [](gin_helper::Promise<v8::Local<v8::Value>> promise,
         std::vector<electron::NotificationInfo> notifications) {
        v8::Isolate* isolate = promise.isolate();
        v8::HandleScope handle_scope(isolate);

        // The browser client may have been torn down by the time this
        // callback fires — null-check to avoid a use-after-free.
        auto* browser_client = ElectronBrowserClient::Get();
        if (!browser_client) {
          promise.Resolve(v8::Array::New(isolate).As<v8::Value>());
          return;
        }

        auto* presenter = static_cast<ElectronBrowserClient*>(browser_client)
                              ->GetNotificationPresenter();
        if (!presenter) {
          promise.Resolve(v8::Array::New(isolate).As<v8::Value>());
          return;
        }

        v8::Local<v8::Array> result =
            v8::Array::New(isolate, notifications.size());
        for (size_t i = 0; i < notifications.size(); i++) {
          const auto& info = notifications[i];

          // Create a restored Notification object for each delivered
          // notification. The API object is owned by V8 GC (via
          // CreateHandle), while CreateNotification creates a separate
          // platform notification owned by the presenter. They are connected
          // by a WeakPtr (API -> platform) and a raw delegate pointer
          // (platform -> API, cleared in ~Notification).
          auto* notif = new Notification(info);
          notif->notification_ = presenter->CreateNotification(
              notif, notif->id_, notif->raw_group_id_);
          if (notif->notification_)
            notif->notification_->Restore();

          auto handle = gin_helper::CreateHandle(isolate, notif);
          result
              ->Set(isolate->GetCurrentContext(), static_cast<uint32_t>(i),
                    handle.ToV8())
              .Check();
        }

        promise.Resolve(result.As<v8::Value>());
      },
      std::move(promise)));

  return handle;
}

void Notification::FillObjectTemplate(v8::Isolate* isolate,
                                      v8::Local<v8::ObjectTemplate> templ) {
  gin::ObjectTemplateBuilder(isolate, GetClassName(), templ)
      .SetMethod("show", &Notification::Show)
      .SetMethod("close", &Notification::Close)
      .SetProperty("id", &Notification::id)
      .SetProperty("groupId", &Notification::group_id)
      .SetProperty("groupTitle", &Notification::group_title)
      .SetProperty("title", &Notification::title, &Notification::SetTitle)
      .SetProperty("subtitle", &Notification::subtitle,
                   &Notification::SetSubtitle)
      .SetProperty("body", &Notification::body, &Notification::SetBody)
      .SetProperty("silent", &Notification::is_silent, &Notification::SetSilent)
      .SetProperty("hasReply", &Notification::has_reply,
                   &Notification::SetHasReply)
      .SetProperty("timeoutType", &Notification::timeout_type,
                   &Notification::SetTimeoutType)
      .SetProperty("replyPlaceholder", &Notification::reply_placeholder,
                   &Notification::SetReplyPlaceholder)
      .SetProperty("urgency", &Notification::urgency, &Notification::SetUrgency)
      .SetProperty("sound", &Notification::sound, &Notification::SetSound)
      .SetProperty("actions", &Notification::actions, &Notification::SetActions)
      .SetProperty("closeButtonText", &Notification::close_button_text,
                   &Notification::SetCloseButtonText)
      .SetProperty("toastXml", &Notification::toast_xml,
                   &Notification::SetToastXml)
      .Build();
}

const char* Notification::GetTypeName() {
  return GetClassName();
}

void Notification::WillBeDestroyed() {
  ClearWeak();
}
}  // namespace electron::api

namespace {

using electron::api::Notification;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* const isolate = electron::JavascriptEnvironment::GetIsolate();
  gin_helper::Dictionary dict{isolate, exports};
  dict.Set("Notification", Notification::GetConstructor(isolate, context));
  dict.SetMethod("isSupported", &Notification::IsSupported);
#if BUILDFLAG(IS_WIN)
  dict.SetMethod("handleActivation", &Notification::HandleActivation);
#endif
  dict.SetMethod("getHistory", &Notification::GetHistory);
}

}  // namespace

NODE_LINKED_BINDING_CONTEXT_AWARE(electron_browser_notification, Initialize)
