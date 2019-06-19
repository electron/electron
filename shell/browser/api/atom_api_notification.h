// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
#define ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/notifications/notification.h"
#include "atom/browser/notifications/notification_delegate.h"
#include "atom/browser/notifications/notification_presenter.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace api {

class Notification : public mate::TrackableObject<Notification>,
                     public NotificationDelegate {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);
  static bool IsSupported();

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // NotificationDelegate:
  void NotificationAction(int index) override;
  void NotificationClick() override;
  void NotificationReplied(const std::string& reply) override;
  void NotificationDisplayed() override;
  void NotificationDestroyed() override;
  void NotificationClosed() override;

 protected:
  Notification(v8::Isolate* isolate,
               v8::Local<v8::Object> wrapper,
               mate::Arguments* args);
  ~Notification() override;

  void Show();
  void Close();

  // Prop Getters
  base::string16 GetTitle() const;
  base::string16 GetSubtitle() const;
  base::string16 GetBody() const;
  bool GetSilent() const;
  bool GetHasReply() const;
  base::string16 GetReplyPlaceholder() const;
  base::string16 GetSound() const;
  std::vector<atom::NotificationAction> GetActions() const;
  base::string16 GetCloseButtonText() const;

  // Prop Setters
  void SetTitle(const base::string16& new_title);
  void SetSubtitle(const base::string16& new_subtitle);
  void SetBody(const base::string16& new_body);
  void SetSilent(bool new_silent);
  void SetHasReply(bool new_has_reply);
  void SetReplyPlaceholder(const base::string16& new_reply_placeholder);
  void SetSound(const base::string16& sound);
  void SetActions(const std::vector<atom::NotificationAction>& actions);
  void SetCloseButtonText(const base::string16& text);

 private:
  base::string16 title_;
  base::string16 subtitle_;
  base::string16 body_;
  gfx::Image icon_;
  base::string16 icon_path_;
  bool has_icon_ = false;
  bool silent_ = false;
  bool has_reply_ = false;
  base::string16 reply_placeholder_;
  base::string16 sound_;
  std::vector<atom::NotificationAction> actions_;
  base::string16 close_button_text_;

  atom::NotificationPresenter* presenter_;

  base::WeakPtr<atom::Notification> notification_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
