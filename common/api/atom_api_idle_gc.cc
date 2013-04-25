// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Copyright (c) 2012, Ben Noordhuis <info@bnoordhuis.nl>.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vendor/node/src/node.h"

namespace {

using v8::Arguments;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Object;
using v8::String;
using v8::Undefined;
using v8::V8;
using v8::Value;

typedef enum { STOP, RUN, PAUSE } gc_state_t;

int64_t interval;
gc_state_t state;
gc_state_t prev_state;
uv_timer_t timer_handle;
uv_check_t check_handle;
uv_prepare_t prepare_handle;

void Timer(uv_timer_t*, int) {
  if (V8::IdleNotification()) state = PAUSE;
}

void Check(uv_check_t*, int) {
  prev_state = state;
}

void Prepare(uv_prepare_t*, int) {
  if (state == PAUSE && prev_state == PAUSE) state = RUN;
  if (state == RUN) uv_timer_start(&timer_handle, Timer, interval, 0);
}

Handle<Value> Stop(const Arguments& args) {
  state = STOP;
  uv_timer_stop(&timer_handle);
  uv_check_stop(&check_handle);
  uv_prepare_stop(&prepare_handle);
  return Undefined();
}

Handle<Value> Start(const Arguments& args) {
  HandleScope scope;
  Stop(args);

  interval = args[0]->IsNumber() ? args[0]->IntegerValue() : 0;
  if (interval <= 0) interval = 5000;  // Default to 5 seconds.

  state = RUN;
  uv_check_start(&check_handle, Check);
  uv_prepare_start(&prepare_handle, Prepare);

  return Undefined();
}

void Init(Handle<Object> obj) {
  HandleScope scope;

  uv_timer_init(uv_default_loop(), &timer_handle);
  uv_check_init(uv_default_loop(), &check_handle);
  uv_prepare_init(uv_default_loop(), &prepare_handle);
  uv_unref(reinterpret_cast<uv_handle_t*>(&timer_handle));
  uv_unref(reinterpret_cast<uv_handle_t*>(&check_handle));
  uv_unref(reinterpret_cast<uv_handle_t*>(&prepare_handle));

  obj->Set(String::New("stop"), FunctionTemplate::New(Stop)->GetFunction());
  obj->Set(String::New("start"), FunctionTemplate::New(Start)->GetFunction());
}

NODE_MODULE(atom_common_idle_gc, Init)

}  // namespace
