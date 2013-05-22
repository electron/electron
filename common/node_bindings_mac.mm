// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/node_bindings_mac.h"

#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include "base/message_loop.h"
#include "content/public/browser/browser_thread.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

using content::BrowserThread;

namespace atom {

namespace {

void UvNoOp(uv_async_t* handle, int status) {
}

}

NodeBindingsMac::NodeBindingsMac(bool is_browser)
    : NodeBindings(is_browser),
      loop_(uv_default_loop()),
      kqueue_(kqueue()),
      embed_closed_(false) {
}

NodeBindingsMac::~NodeBindingsMac() {
  // Clear uv.
  embed_closed_ = true;
  uv_thread_join(&embed_thread_);
  uv_sem_destroy(&embed_sem_);
}

void NodeBindingsMac::PrepareMessageLoop() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Add dummy handle for libuv, otherwise libuv would quit when there is
  // nothing to do.
  uv_async_init(loop_, &dummy_uv_handle_, UvNoOp);

  // Start worker that will interrupt main loop when having uv events.
  uv_sem_init(&embed_sem_, 0);
  uv_thread_create(&embed_thread_, EmbedThreadRunner, this);
}

void NodeBindingsMac::RunMessageLoop() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  loop_->data = this;
  loop_->on_watcher_queue_updated = OnWatcherQueueChanged;

  // Run uv loop for once to give the uv__io_poll a chance to add all events.
  UvRunOnce();
}

void NodeBindingsMac::UvRunOnce() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Enter node context while dealing with uv events.
  v8::HandleScope scope;
  v8::Context::Scope context_scope(node::g_context);

  // Deal with uv events.
  int r = uv_run(loop_, (uv_run_mode)(UV_RUN_ONCE | UV_RUN_NOWAIT));
  if (r == 0 || loop_->stop_flag != 0)
    MessageLoop::current()->QuitWhenIdle();  // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindingsMac::EmbedThreadRunner(void *arg) {
  NodeBindingsMac* self = static_cast<NodeBindingsMac*>(arg);

  uv_loop_t* loop = self->loop_;

  // Add uv's backend fd to kqueue.
  struct kevent ev;
  EV_SET(&ev, uv_backend_fd(loop), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
  kevent(self->kqueue_, &ev, 1, NULL, 0, NULL);

  while (!self->embed_closed_) {
    // Wait for the main loop to deal with events.
    uv_sem_wait(&self->embed_sem_);

    struct timespec spec;
    int timeout = uv_backend_timeout(loop);
    if (timeout != -1) {
      spec.tv_sec = timeout / 1000;
      spec.tv_nsec = (timeout % 1000) * 1000000;
    }

    // Wait for new libuv events.
    int r;
    do {
      r = ::kevent(self->kqueue_, NULL, 0, &ev, 1,
                   timeout == -1 ? NULL : &spec);
    } while (r == -1 && errno == EINTR);

    // Deal with event in main thread.
    dispatch_async(dispatch_get_main_queue(), ^{
      self->UvRunOnce();
    });
  }
}

// static
void NodeBindingsMac::OnWatcherQueueChanged(uv_loop_t* loop) {
  NodeBindingsMac* self = static_cast<NodeBindingsMac*>(loop->data);

  DCHECK(!self->is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // We need to break the io polling in the kqueue thread when loop's watcher
  // queue changes, otherwise new events cannot be notified.
  uv_async_send(&self->dummy_uv_handle_);
}

// static
NodeBindings* NodeBindings::Create(bool is_browser) {
  return new NodeBindingsMac(is_browser);
}

}  // namespace atom
