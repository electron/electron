// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_auto_updater.h"

#include "base/time/time.h"
#include "base/values.h"
#include "atom/browser/auto_updater.h"
#include "atom/common/v8/native_type_conversions.h"

#include "atom/common/node_includes.h"

namespace atom {

namespace api {

AutoUpdater::AutoUpdater(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper) {
  auto_updater::AutoUpdater::SetDelegate(this);
}

AutoUpdater::~AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(NULL);
}

void AutoUpdater::OnError(const std::string& error) {
  base::ListValue args;
  args.AppendString(error);
  Emit("error", &args);
}

void AutoUpdater::OnCheckingForUpdate() {
  Emit("checking-for-update");
}

void AutoUpdater::OnUpdateAvailable() {
  Emit("update-available");
}

void AutoUpdater::OnUpdateNotAvailable() {
  Emit("update-not-available");
}

void AutoUpdater::OnUpdateDownloaded(const std::string& release_notes,
                                     const std::string& release_name,
                                     const base::Time& release_date,
                                     const std::string& update_url,
                                     const base::Closure& quit_and_install) {
  quit_and_install_ = quit_and_install;

  base::ListValue args;
  args.AppendString(release_notes);
  args.AppendString(release_name);
  args.AppendDouble(release_date.ToJsTime());
  args.AppendString(update_url);
  Emit("update-downloaded-raw", &args);
}

// static
void AutoUpdater::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  new AutoUpdater(args.This());
}

// static
void AutoUpdater::SetFeedURL(const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto_updater::AutoUpdater::SetFeedURL(FromV8Value(args[0]));
}

// static
void AutoUpdater::CheckForUpdates(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto_updater::AutoUpdater::CheckForUpdates();
}

// static
void AutoUpdater::QuitAndInstall(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  AutoUpdater* self = AutoUpdater::Unwrap<AutoUpdater>(args.This());

  if (!self->quit_and_install_.is_null())
    self->quit_and_install_.Run();
}

// static
void AutoUpdater::Initialize(v8::Handle<v8::Object> target) {
  v8::Local<v8::FunctionTemplate> t(
      v8::FunctionTemplate::New(AutoUpdater::New));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("AutoUpdater"));

  NODE_SET_PROTOTYPE_METHOD(t, "setFeedUrl", SetFeedURL);
  NODE_SET_PROTOTYPE_METHOD(t, "checkForUpdates", CheckForUpdates);
  NODE_SET_PROTOTYPE_METHOD(t, "quitAndInstall", QuitAndInstall);

  target->Set(v8::String::NewSymbol("AutoUpdater"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_auto_updater, atom::api::AutoUpdater::Initialize)
