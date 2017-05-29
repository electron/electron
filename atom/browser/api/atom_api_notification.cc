// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_notification.h"

#include <map>
#include <string>

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/browser.h"
#include "atom/browser/ui/notification_delegate_adapter.h"
#include "atom/common/api/atom_api_native_image.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "native_mate/constructor.h"
#include "native_mate/dictionary.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace atom {

namespace api {

int id_counter = 1;
std::map<int, Notification*> notifications_;

Notification::Notification(v8::Isolate* isolate,
                           v8::Local<v8::Object> wrapper,
                           mate::Arguments* args) {
  InitWith(isolate, wrapper);

  mate::Dictionary opts;
  if (args->GetNext(&opts)) {
    opts.Get("title", &title_);
    opts.Get("body", &body_);
    has_icon_ = opts.Get("icon", &icon_);
    if (has_icon_) {
      opts.Get("icon", &icon_path_);
    }
    opts.Get("silent", &silent_);
    opts.Get("replyPlaceholder", &reply_placeholder_);
    opts.Get("hasReply", &has_reply_);
    id_ = id_counter++;
  }
  notifications_[id_] = this;
  OnInitialProps();
}

Notification::~Notification() {}

// static
mate::WrappableBase* Notification::New(mate::Arguments* args) {
  if (!Browser::Get()->is_ready()) {
    args->ThrowError("Cannot create Notification before app is ready");
    return nullptr;
  }
  return new Notification(args->isolate(), args->GetThis(), args);
}

bool Notification::HasID(int id) {
  return notifications_.find(id) != notifications_.end();
}

Notification* Notification::FromID(int id) {
  return notifications_[id];
}

// Getters
int Notification::GetID() {
  return id_;
}
base::string16 Notification::GetTitle() {
  return title_;
}
base::string16 Notification::GetBody() {
  return body_;
}
bool Notification::GetSilent() {
  return silent_;
}
base::string16 Notification::GetReplyPlaceholder() {
  return reply_placeholder_;
}
bool Notification::GetHasReply() {
  return has_reply_;
}

// Setters
void Notification::SetTitle(base::string16 new_title) {
  title_ = new_title;
  NotifyPropsUpdated();
}
void Notification::SetBody(base::string16 new_body) {
  body_ = new_body;
  NotifyPropsUpdated();
}
void Notification::SetSilent(bool new_silent) {
  silent_ = new_silent;
  NotifyPropsUpdated();
}
void Notification::SetReplyPlaceholder(base::string16 new_reply_placeholder) {
  reply_placeholder_ = new_reply_placeholder;
  NotifyPropsUpdated();
}
void Notification::SetHasReply(bool new_has_reply) {
  has_reply_ = new_has_reply;
  NotifyPropsUpdated();
}

void Notification::OnClicked() {
  Emit("click");
}

void Notification::OnReplied(std::string reply) {
  Emit("reply", reply);
}

void Notification::OnShown() {
  Emit("show");
}

void Notification::OnClosed() {
  Emit("close");
}

void Notification::NotifyPropsUpdated() {}

// Showing notifications
void Notification::Show() {
  SkBitmap image = *(new SkBitmap);
  if (has_icon_) {
    image = *(icon_.ToSkBitmap());
  }

  std::unique_ptr<AtomNotificationDelegateAdapter> adapter(
      new AtomNotificationDelegateAdapter(this));
  auto notif = presenter_->CreateNotification(adapter.get());
  if (notif) {
    ignore_result(adapter.release());  // it will release itself automatically.
    GURL nullUrl = *(new GURL);
    notif->Show(title_, body_, "", nullUrl, image, silent_, has_reply_, reply_placeholder_);
  }
}

bool initialized_ = false;
void Notification::OnInitialProps() {
  if (!initialized_) {
    presenter_ = brightray::NotificationPresenter::Create();
    initialized_ = true;
  }
}

// static
void Notification::BuildPrototype(v8::Isolate* isolate,
                                  v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "Notification"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("show", &Notification::Show)
      .SetProperty("id", &Notification::GetID)
      .SetProperty("title", &Notification::GetTitle, &Notification::SetTitle)
      .SetProperty("body", &Notification::GetBody, &Notification::SetBody)
      .SetProperty("silent", &Notification::GetSilent, &Notification::SetSilent)
      .SetProperty("replyPlaceholder", &Notification::GetReplyPlaceholder,
                   &Notification::SetReplyPlaceholder)
      .SetProperty("hasReply", &Notification::GetHasReply,
                   &Notification::SetHasReply);
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::Notification;

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  Notification::SetConstructor(isolate, base::Bind(&Notification::New));

  mate::Dictionary dict(isolate, exports);
  dict.Set("Notification",
           Notification::GetConstructor(isolate)->GetFunction());
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_notification, Initialize)
