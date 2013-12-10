// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_auto_updater.h"

#include "base/values.h"
#include "browser/auto_updater.h"
#include "common/v8_conversions.h"

namespace atom {

namespace api {

AutoUpdater::AutoUpdater(v8::Handle<v8::Object> wrapper)
    : EventEmitter(wrapper) {
  auto_updater::AutoUpdater::SetDelegate(this);
  auto_updater::AutoUpdater::Init();
}

AutoUpdater::~AutoUpdater() {
  auto_updater::AutoUpdater::SetDelegate(NULL);
}

void AutoUpdater::WillInstallUpdate(const std::string& version,
                                    const base::Closure& install) {
  continue_update_ = install;

  base::ListValue args;
  args.AppendString(version);
  bool prevent_default = Emit("will-install-update-raw", &args);

  if (!prevent_default)
    install.Run();
}

void AutoUpdater::ReadyForUpdateOnQuit(const std::string& version,
                                       const base::Closure& quit_and_install) {
  quit_and_install_ = quit_and_install;

  base::ListValue args;
  args.AppendString(version);
  Emit("ready-for-update-on-quit-raw", &args);
}

// static
v8::Handle<v8::Value> AutoUpdater::New(const v8::Arguments& args) {
  v8::HandleScope scope;

  if (!args.IsConstructCall())
    return node::ThrowError("Require constructor call");

  new AutoUpdater(args.This());

  return args.This();
}

// static
v8::Handle<v8::Value> AutoUpdater::SetFeedURL(const v8::Arguments& args) {
  auto_updater::AutoUpdater::SetFeedURL(FromV8Value(args[0]));
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::SetAutomaticallyChecksForUpdates(
      const v8::Arguments& args) {
  auto_updater::AutoUpdater::SetAutomaticallyChecksForUpdates(
      args[0]->BooleanValue());
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::SetAutomaticallyDownloadsUpdates(
      const v8::Arguments& args) {
  auto_updater::AutoUpdater::SetAutomaticallyDownloadsUpdates(
      args[0]->BooleanValue());
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::CheckForUpdates(const v8::Arguments& args) {
  auto_updater::AutoUpdater::CheckForUpdates();
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::CheckForUpdatesInBackground(
      const v8::Arguments& args) {
  auto_updater::AutoUpdater::CheckForUpdatesInBackground();
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::ContinueUpdate(const v8::Arguments& args) {
  AutoUpdater* self = AutoUpdater::Unwrap<AutoUpdater>(args.This());
  self->continue_update_.Run();
  return v8::Undefined();
}

// static
v8::Handle<v8::Value> AutoUpdater::QuitAndInstall(const v8::Arguments& args) {
  AutoUpdater* self = AutoUpdater::Unwrap<AutoUpdater>(args.This());
  self->quit_and_install_.Run();
  return v8::Undefined();
}

// static
void AutoUpdater::Initialize(v8::Handle<v8::Object> target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t(
      v8::FunctionTemplate::New(AutoUpdater::New));
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(v8::String::NewSymbol("AutoUpdater"));

  NODE_SET_PROTOTYPE_METHOD(t, "setFeedUrl", SetFeedURL);
  NODE_SET_PROTOTYPE_METHOD(t,
                            "setAutomaticallyChecksForUpdates",
                            SetAutomaticallyChecksForUpdates);
  NODE_SET_PROTOTYPE_METHOD(t,
                            "setAutomaticallyDownloadsUpdates",
                            SetAutomaticallyDownloadsUpdates);
  NODE_SET_PROTOTYPE_METHOD(t, "checkForUpdates", CheckForUpdates);
  NODE_SET_PROTOTYPE_METHOD(t,
                            "checkForUpdatesInBackground",
                            CheckForUpdatesInBackground);

  NODE_SET_PROTOTYPE_METHOD(t, "continueUpdate", ContinueUpdate);
  NODE_SET_PROTOTYPE_METHOD(t, "quitAndInstall", QuitAndInstall);

  target->Set(v8::String::NewSymbol("AutoUpdater"), t->GetFunction());
}

}  // namespace api

}  // namespace atom

NODE_MODULE(atom_browser_auto_updater, atom::api::AutoUpdater::Initialize)
