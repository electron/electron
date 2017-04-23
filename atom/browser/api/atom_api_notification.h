// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
#define ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/ui/notification_observer.h"
#include "base/strings/utf_string_conversions.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

namespace atom {

namespace api {

class Notification : public mate::TrackableObject<Notification>,
             public NotifictionObserver {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);
  static bool HasID(int id);
  static Notification* FromID(int id);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // NotificationObserver:
  void OnClicked() override;
  void OnReplied(std::string reply) override;
  void OnShown() override;

 protected:
  Notification(v8::Isolate* isolate, v8::Local<v8::Object> wrapper, mate::Arguments* args);
  ~Notification() override;

  void OnInitialProps();
  void Show();

  // Prop Getters
  int GetID();
  base::string16 GetTitle();
  base::string16 GetBody();
  bool GetSilent();
  base::string16 GetReplyPlaceholder();
  bool GetHasReply();

  // Prop Setters
  void SetTitle(base::string16 new_title);
  void SetBody(base::string16 new_body);
  void SetSilent(bool new_silent);
  void SetReplyPlaceholder(base::string16 new_reply_placeholder);
  void SetHasReply(bool new_has_reply);

  void NotifyPropsUpdated();

 private:
  base::string16 title_ = base::UTF8ToUTF16("");
  base::string16 body_ = base::UTF8ToUTF16("");
  gfx::Image icon_;
  base::string16 icon_path_ = base::UTF8ToUTF16("");
  bool has_icon_ = false;
  bool silent_ = false;
  base::string16 reply_placeholder_ = base::UTF8ToUTF16("");
  bool has_reply_ = false;

  int id_;

  DISALLOW_COPY_AND_ASSIGN(Notification);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_NOTIFICATION_H_
