// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_NOTIFICATION_H_
#define SHELL_BROWSER_API_ELECTRON_API_NOTIFICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "ui/gfx/image/image.h"

namespace gin {
class Arguments;
template <typename T>
class Handle;
}  // namespace gin

namespace electron {

namespace api {

class Notification : public gin::Wrappable<Notification>,
                     public gin_helper::EventEmitterMixin<Notification>,
                     public gin_helper::Constructible<Notification>,
                     public gin_helper::CleanedUpAtExit,
                     public NotificationDelegate {
 public:
  static bool IsSupported();

  // gin_helper::Constructible
  static gin::Handle<Notification> New(gin_helper::ErrorThrower thrower,
                                       gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);

  // NotificationDelegate:
  void NotificationAction(int index) override;
  void NotificationClick() override;
  void NotificationReplied(const std::string& reply) override;
  void NotificationDisplayed() override;
  void NotificationDestroyed() override;
  void NotificationClosed() override;
  void NotificationFailed(const std::string& error) override;

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;

 protected:
  explicit Notification(gin::Arguments* args);
  ~Notification() override;

  void Show();
  void Close();

  // Prop Getters
  base::string16 GetTitle() const;
  base::string16 GetSubtitle() const;
  base::string16 GetBody() const;
  bool GetSilent() const;
  bool GetHasReply() const;
  base::string16 GetTimeoutType() const;
  base::string16 GetReplyPlaceholder() const;
  base::string16 GetUrgency() const;
  base::string16 GetSound() const;
  std::vector<electron::NotificationAction> GetActions() const;
  base::string16 GetCloseButtonText() const;
  base::string16 GetToastXml() const;

  // Prop Setters
  void SetTitle(const base::string16& new_title);
  void SetSubtitle(const base::string16& new_subtitle);
  void SetBody(const base::string16& new_body);
  void SetSilent(bool new_silent);
  void SetHasReply(bool new_has_reply);
  void SetUrgency(const base::string16& new_urgency);
  void SetTimeoutType(const base::string16& new_timeout_type);
  void SetReplyPlaceholder(const base::string16& new_reply_placeholder);
  void SetSound(const base::string16& sound);
  void SetActions(const std::vector<electron::NotificationAction>& actions);
  void SetCloseButtonText(const base::string16& text);
  void SetToastXml(const base::string16& new_toast_xml);

 private:
  base::string16 title_;
  base::string16 subtitle_;
  base::string16 body_;
  gfx::Image icon_;
  base::string16 icon_path_;
  bool has_icon_ = false;
  bool silent_ = false;
  bool has_reply_ = false;
  base::string16 timeout_type_;
  base::string16 reply_placeholder_;
  base::string16 sound_;
  base::string16 urgency_;
  std::vector<electron::NotificationAction> actions_;
  base::string16 close_button_text_;
  base::string16 toast_xml_;

  electron::NotificationPresenter* presenter_;

  base::WeakPtr<electron::Notification> notification_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_NOTIFICATION_H_
