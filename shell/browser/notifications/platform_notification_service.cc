// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#include "shell/browser/notifications/platform_notification_service.h"

#include <algorithm>
#include <set>
#include <utility>
#include "content/public/browser/notification_event_dispatcher.h"
#include "content/public/browser/render_process_host.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "third_party/blink/public/common/notifications/notification_resources.h"
#include "third_party/blink/public/common/notifications/platform_notification_data.h"
#include "third_party/blink/public/mojom/notifications/notification.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/strings/utf_string_conversions.h"

#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8-platform.h"
#include "v8/include/v8-value-serializer.h"
#include "v8/include/v8.h"

namespace electron {

namespace {

void OnWebNotificationAllowed(base::WeakPtr<Notification> notification,
                              const SkBitmap& icon,
                              const SkBitmap& image,
                              const blink::PlatformNotificationData& data,
                              bool is_persistent,
                              bool is_replacing,
                              bool audio_muted,
                              bool allowed) {
  if (!notification)
    return;
  if (allowed) {
    electron::NotificationOptions options;
    options.title = data.title;
    options.msg = data.body;
    options.tag = data.tag;
    options.icon_url = data.icon;
    options.icon = icon;
    options.image_url = data.image;
    options.image = image;
    options.silent = audio_muted ? true : data.silent;
    options.has_reply = false;
    // feat: Correct processing for the persistent notification without
    // actions
    options.is_persistent = is_persistent;
    // feat: Configuration toast show time according to
    // Notification.requireInteraction property
    options.require_interaction = data.require_interaction;
    // feat: Display toast according to Notification.renotify
    options.should_be_presented = data.renotify ? true : !is_replacing;
    if (!data.data.empty()) {
      // feat: support Notification.data property
      v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
      // Create a stack-allocated handle scope.
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> context = isolate->GetCurrentContext();

      v8::ValueDeserializer deserializer(
          isolate, reinterpret_cast<const uint8_t*>(data.data.data()),
          data.data.size());

      v8::Maybe<bool> is_header_valid = deserializer.ReadHeader(context);
      if (is_header_valid.IsNothing()) {
        // see
        // third_party\blink\renderer\bindings\core\v8\serialization\v8_script_value_deserializer.cc
        // V8ScriptValueDeserializer::Deserialize -> ReadVersionEnvelope
        // we have case with blink extra data and need to skip some portion of
        // data till valid header start
        auto begin = std::begin(data.data);
        auto last = std::end(data.data);

        auto find = std::find_if(std::next(begin), last, [](const char ch) {
          // //v8/src/objects/value-serializer.cc
          // enum class SerializationTag : uint8_t
          // `0xFF` - Version tag
          return static_cast<const uint8_t>(ch) == 0xFF;
        });

        if (find != last) {
          size_t count = std::distance(begin, find);
          const void* bytes;
          // we have 1 bytes from inital ReadHeader call
          // now we need to set internal pointer on byte
          // before version tag to allow ReadHeader again
          if (deserializer.ReadRawBytes(count - 2, &bytes)) {
            is_header_valid = deserializer.ReadHeader(context);
          }
        }
      }
      v8::MaybeLocal<v8::Value> str_value = deserializer.ReadValue(context);

      v8::Local<v8::Value> result;
      if (!is_header_valid.IsNothing() && is_header_valid.ToChecked() &&
          str_value.ToLocal(&result)) {
        v8::String::Utf8Value utf8(isolate, result);
        const char* bgn = utf8.operator*();
        const char* end = bgn + utf8.length();
        std::transform(bgn, end, std::back_inserter(options.data),
                       [](const char ch) { return ch; });
      }
    }
    // feat: Add the reply field on a notification
    auto& has_reply = options.has_reply;
    // feat: Support for actions(buttons) in toast
    std::transform(
        data.actions.begin(), data.actions.end(),
        std::back_inserter(options.actions),
        [&has_reply](const blink::mojom::NotificationActionPtr& action_) {
          // feat: Add the reply field on a notification
          NotificationAction action;
          if (action_) {
            switch (action_->type) {
              case blink::mojom::NotificationActionType::BUTTON:
                action.type = electron::NotificationAction::sTYPE_BUTTON;
                break;
              case blink::mojom::NotificationActionType::TEXT:
                action.type = electron::NotificationAction::sTYPE_TEXT;
                break;
            }
            action.text = action_->title;
            action.icon = action_->icon;
            action.arg = std::u16string(action_->action.size(), 0);
            std::transform(action_->action.begin(), action_->action.end(),
                           action.arg.begin(),
                           [](const char ch) { return ch; });
            has_reply = has_reply || (action.type ==
                                      electron::NotificationAction::sTYPE_TEXT);
            if (action_->placeholder.has_value())
              action.placeholder = action_->placeholder.value();
          }
          return action;
        });
    notification->Show(options);
  } else {
    notification->Destroy();
  }
}
// feat: Upgrade for NotificationDelegateImpl
class NotificationDelegateImpl final : public electron::NotificationDelegate {
 public:
  explicit NotificationDelegateImpl(const std::string& notification_id,
                                    const GURL& origin,
                                    const bool is_persistent)
      : notification_id_(notification_id),
        context_(nullptr),
        origin_(origin),
        is_persistent_(is_persistent) {}

  void SetBrowserContext(content::BrowserContext* context) {
    context_ = context;
  }

  // disable copy
  NotificationDelegateImpl(const NotificationDelegateImpl&) = delete;
  NotificationDelegateImpl& operator=(const NotificationDelegateImpl&) = delete;

  void NotificationDestroyed() final { delete this; }

  void NotificationClick() final {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNonPersistentClickEvent(notification_id_, base::DoNothing());
  }

  void NotificationClosed() final {
    if (is_persistent_) {
      content::NotificationEventDispatcher::GetInstance()
          ->DispatchNotificationCloseEvent(context_, notification_id_, origin_,
                                           true, base::DoNothing());
    } else {
      content::NotificationEventDispatcher::GetInstance()
          ->DispatchNonPersistentCloseEvent(notification_id_,
                                            base::DoNothing());
    }
  }

  void NotificationAction(int index) final {
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNotificationClickEvent(context_, notification_id_, origin_,
                                         index, std::u16string(),
                                         base::DoNothing());
  }

  void NotificationReplied(const std::string& reply) final {
    // feat: Add the reply field on a notification
    content::NotificationEventDispatcher::GetInstance()
        ->DispatchNotificationClickEvent(context_, notification_id_, origin_, 0,
                                         base::UTF8ToUTF16(reply),
                                         base::DoNothing());
  }

 private:
  std::string notification_id_;
  raw_ptr<content::BrowserContext>
      context_;  // context is necessary for Event dispatching
  GURL origin_;
  const bool is_persistent_;
};

}  // namespace

PlatformNotificationService::PlatformNotificationService(
    ElectronBrowserClient* browser_client)
    : browser_client_(browser_client), next_persistent_id_(0) {
  // WTF::Partitions::Initialize();
  // WTF::Partitions::InitializeArrayBufferPartition();
}

PlatformNotificationService::~PlatformNotificationService() = default;

void PlatformNotificationService::DisplayNotification(
    content::RenderFrameHost* render_frame_host,
    const std::string& notification_id,
    const GURL& origin,
    const GURL& document_url,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;

  // If a new notification is created with the same tag as an
  // existing one, replace the old notification with the new one.
  // The notification_id is generated from the tag, so the only way a
  // notification will be closed as a result of this call is if one with
  // the same tag is already extant.
  //
  // See: https://notifications.spec.whatwg.org/#showing-a-notification
  const size_t prev_notif_count = presenter->notifications().size();
  const bool try_pending_deletion(true);
  presenter->CloseNotificationWithId(notification_id, try_pending_deletion);
  const size_t curr_notif_count = presenter->notifications().size();

  const bool is_persistent(false);
  auto* delegate =
      new NotificationDelegateImpl(notification_id, origin, is_persistent);

  auto notification = presenter->CreateNotification(delegate, notification_id);
  if (notification) {
    // feat: correct processing for the persistent notification without
    // actions
    const bool is_replacing(prev_notif_count > curr_notif_count);
    browser_client_->WebNotificationAllowed(
        // content::WebContents::FromRenderFrameHost(render_frame_host),
        render_frame_host,
        base::BindRepeating(&OnWebNotificationAllowed, notification,
                            notification_resources.notification_icon,
                            notification_resources.image, notification_data,
                            is_persistent, is_replacing));
  }
}

void PlatformNotificationService::DisplayPersistentNotification(
    const std::string& notification_id,
    const GURL& service_worker_scope,
    const GURL& origin,
    const blink::PlatformNotificationData& notification_data,
    const blink::NotificationResources& notification_resources) {
  absl::optional<int> proc_id =
      browser_client_->GetRenderFrameProcessID(service_worker_scope);
  if (!proc_id.has_value())
    return;

  content::WebContents* web_ctx =
      browser_client_->GetWebContentsFromProcessID(proc_id.value());
  if (!web_ctx)
    return;

  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;

  const size_t prev_notif_count = presenter->notifications().size();
  const bool try_pending_deletion(true);
  presenter->CloseNotificationWithId(notification_id, try_pending_deletion);
  const size_t curr_notif_count = presenter->notifications().size();

  const bool is_persistent(true);
  auto* delegate =
      new NotificationDelegateImpl(notification_id, origin, is_persistent);

  auto notification = presenter->CreateNotification(delegate, notification_id);
  if (notification) {
    delegate->SetBrowserContext(web_ctx->GetBrowserContext());
    // feat: Correct processing for the persistent notification without
    // actions
    const bool is_replacing(prev_notif_count > curr_notif_count);
    browser_client_->WebNotificationAllowed(
        web_ctx->GetPrimaryMainFrame(),
        base::BindRepeating(&OnWebNotificationAllowed, notification,
                            notification_resources.notification_icon,
                            notification_resources.image, notification_data,
                            is_persistent, is_replacing));
    // OnWebNotificationAllowed( notification,
    //     notification_resources.notification_icon,
    //     notification_data, is_persistent, false, true);
  }
}

void PlatformNotificationService::ClosePersistentNotification(
    const std::string& notification_id) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  presenter->CloseNotificationWithId(notification_id);
}

void PlatformNotificationService::CloseNotification(
    const std::string& notification_id) {
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;
  presenter->CloseNotificationWithId(notification_id);
}

void PlatformNotificationService::GetDisplayedNotifications(
    DisplayedNotificationsCallback callback) {
  // feat: Expose a call to clear all notifications
  auto* presenter = browser_client_->GetNotificationPresenter();
  if (!presenter)
    return;

  if (callback) {
    auto notifications = presenter->notifications();
    std::set<std::string> displayed_notifications;
    std::transform(
        notifications.begin(), notifications.end(),
        std::inserter(displayed_notifications, displayed_notifications.begin()),
        [](const electron::Notification* notification) {
          return notification ? notification->notification_id() : "![DELETE]";
        });
    displayed_notifications.erase("![DELETE]");
    const bool supports_synchronization(true);
    std::move(callback).Run(displayed_notifications, supports_synchronization);
  }
}

int64_t PlatformNotificationService::ReadNextPersistentNotificationId() {
  // feat: Current version of electron supports persistent notifications.
  return next_persistent_id_++;
}

void PlatformNotificationService::RecordNotificationUkmEvent(
    const content::NotificationDatabaseData& data) {}

void PlatformNotificationService::ScheduleTrigger(base::Time timestamp) {}

base::Time PlatformNotificationService::ReadNextTriggerTimestamp() {
  return base::Time::Max();
}

}  // namespace electron
