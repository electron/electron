// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "node_bindings_mac.h"

#include "base/message_loop.h"
#include "content/public/browser/browser_thread.h"
#include "vendor/node/src/node.h"
#include "vendor/node/src/node_internals.h"

#define UV__POLLIN   1
#define UV__POLLOUT  2
#define UV__POLLERR  4

using content::BrowserThread;

namespace atom {

namespace {

void UvNoOp(uv_async_t* handle, int status) {
}

}

NodeBindingsMac::NodeBindingsMac(bool is_browser)
    : NodeBindings(is_browser),
      loop_(uv_default_loop()),
      embed_closed_(false),
      has_pending_event_(false) {
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

void NodeBindingsMac::OnKqueueHasNewEvents() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  DealWithPendingEvent();
  UvRunOnce();
}

void NodeBindingsMac::UvRunOnce() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  // Enter node context while dealing with uv events.
  v8::Context::Scope context_scope(node::g_context);

  // Deal with uv events.
  int r = uv_run(loop_, (uv_run_mode)(UV_RUN_ONCE | UV_RUN_NOWAIT));
  if (r == 0 || loop_->stop_flag != 0)
    MessageLoop::current()->QuitWhenIdle(); // Quit from uv.

  // Tell the worker thread to continue polling.
  uv_sem_post(&embed_sem_);
}

void NodeBindingsMac::DealWithPendingEvent() {
  DCHECK(!is_browser_ || BrowserThread::CurrentlyOn(BrowserThread::UI));

  if (has_pending_event_) {
    has_pending_event_ = false;

    // Enter node context while dealing with uv events.
    v8::Context::Scope context_scope(node::g_context);

    // The pending event must be a one shot event, so we don't need to
    // delete it from kqueue.
    uv__io_t* w = loop_->watchers[event_.ident];
    if (w) {
      unsigned int revents;

      if (event_.filter == EVFILT_VNODE) {
        w->cb(loop_, w, event_.fflags);
        return;
      }

      revents = 0;

      if (event_.filter == EVFILT_READ && (w->events & UV__POLLIN)) {
        revents |= UV__POLLIN;
        w->rcount = event_.data;
      }

      if (event_.filter == EVFILT_WRITE && (w->events & UV__POLLOUT)) {
        revents |= UV__POLLOUT;
        w->wcount = event_.data;
      }

      if (event_.flags & EV_ERROR)
        revents |= UV__POLLERR;

      if (revents == 0)
        return;

      w->cb(loop_, w, revents);
    }
  }
}

void NodeBindingsMac::EmbedThreadRunner(void *arg) {
  NodeBindingsMac* self = static_cast<NodeBindingsMac*>(arg);

  while (!self->embed_closed_) {
    // Wait for the main loop to deal with events.
    uv_sem_wait(&self->embed_sem_);

    uv_loop_t* loop = self->loop_;

    struct timespec spec;
    int timeout = uv_backend_timeout(loop);
    if (timeout != -1) {
      spec.tv_sec = timeout / 1000;
      spec.tv_nsec = (timeout % 1000) * 1000000;
    }

    // Wait for new libuv events.
    int r;
    do {
      r = ::kevent(uv_backend_fd(loop), NULL, 0, &self->event_, 1,
                   timeout == -1 ? NULL : &spec);
    } while (r == -1 && errno == EINTR);

    // We captured a one shot event, we should deal it in main thread because
    // next kevent call will not catch it.
    if (r == 1 && (self->event_.flags & EV_ONESHOT))
      self->has_pending_event_ = true;

    // Deal with event in main thread.
    dispatch_async(dispatch_get_main_queue(), ^{
      self->OnKqueueHasNewEvents();
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
