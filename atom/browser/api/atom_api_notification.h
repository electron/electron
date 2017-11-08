// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
#define ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "base/strings/utf_string_conversions.h"
#include "brightray/browser/notification.h"
#include "brightray/browser/notification_delegate.h"
#include "brightray/browser/notification_presenter.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace api {

class Notification : public mate::TrackableObject<Notification>,
                     public brightray::NotificationDelegate {
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
  base::string16 GetReplyPlaceholder() const;
  bool GetHasReply() const;
  std::vector<brightray::NotificationAction> GetActions() const;
  base::string16 GetSound() const;

  // Prop Setters
  void SetTitle(const base::string16& new_title);
  void SetSubtitle(const base::string16& new_subtitle);
  void SetBody(const base::string16& new_body);
  void SetSilent(bool new_silent);
  void SetReplyPlaceholder(const base::string16& new_reply_placeholder);
  void SetHasReply(bool new_has_reply);
  void SetActions(const std::vector<brightray::NotificationAction>& actions);
  void SetSound(const base::string16& sound);

 private:
  base::string16 title_;
  base::string16 subtitle_;
  base::string16 body_;
  gfx::Image icon_;
  base::string16 icon_path_;
  bool has_icon_ = false;
  bool silent_ = false;
  base::string16 reply_placeholder_;
  bool has_reply_ = false;
  std::vector<brightray::NotificationAction> actions_;
  base::string16 sound_;

  brightray::NotificationPresenter* presenter_;

  base::WeakPtr<brightray::Notification> notification_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
