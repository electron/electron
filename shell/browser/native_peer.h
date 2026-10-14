// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NATIVE_PEER_H_
#define ELECTRON_SHELL_BROWSER_NATIVE_PEER_H_

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "base/check.h"
#include "shell/browser/microtasks_runner.h"
#include "v8/include/cppgc/macros.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/cppgc/type-traits.h"

namespace electron {

// The off heap half of a cppgc managed API wrapper.
//
// A wrapper's destructor runs during cppgc sweeping, where it must not touch
// other GC objects, run JavaScript, or tear down native registrations. A
// native peer owns everything that can outlive wrapper reachability: native
// observer/client/delegate registrations, native resources, and independent
// V8 roots such as pending promises. The wrapper holds the peer through
// NativePeer<Wrapper>::Ptr, whose deleter only queues it, so peer teardown
// never runs inside a cppgc finalizer.
//
// Every path ends in Release(), which runs once, outside sweeping, while the
// isolate is alive:
//   - Explicit: the wrapper calls Release(), e.g. from destroy().
//   - Shutdown: MicrotasksRunner calls OnShutdown(), then Release().
//   - Collection: the wrapper is destroyed, the deleter queues the peer, and a
//     posted task calls Release() and deletes it. At shutdown, queued peers
//     are released before the isolate is disposed and deleted after it.
//
// Release() always runs in this order:
//   1. The peer stops being active; is_active() is false, so peers refuse new
//      native work and reentrant Release() calls return immediately.
//   2. Shutdown observation stops.
//   3. TearDownNative() releases native registrations and resources. The
//      wrapper link is still connected, so callbacks fired while native
//      objects are destroyed can still reach a live wrapper. A peer that must
//      not forward them calls DisconnectWrapper() first.
//   4. The wrapper link is disconnected.
//   5. The peer is released and inert; only deletion remains, which may
//      happen after isolate disposal.
//
// Release() tears down synchronously. Callers on a native callback stack
// whose teardown would destroy the notifying object must post it instead.
//
// Peers are single sequence objects owned by the thread of their isolate.
class NativePeerBase : public MicrotasksRunner::Observer {
 public:
  struct Deleter {
    void operator()(NativePeerBase* peer) const;
  };

  NativePeerBase(const NativePeerBase&) = delete;
  NativePeerBase& operator=(const NativePeerBase&) = delete;

  // Called once MicrotasksRunner shutdown observers have run, while V8 can
  // still run. Releases and deletes every queued peer. Peers queued after this
  // are deleted by DeleteQueuedPeersAfterIsolateDisposal().
  static void ReleaseQueuedPeersForShutdown();

  // Called after the isolate and its heap are gone. Deletes peers whose
  // wrappers were destroyed during heap teardown.
  static void DeleteQueuedPeersAfterIsolateDisposal();

  bool is_active() const { return state_ == State::kActive; }
  bool is_released() const { return state_ == State::kReleased; }

  // Registers for MicrotasksRunner shutdown notification. Called explicitly
  // so each peer keeps its intended position in the LIFO shutdown order.
  void StartObservingShutdown();

  void Release();

 protected:
  NativePeerBase();
  ~NativePeerBase() override;

  // Step 3 of Release(). May settle promises and so run JavaScript. Must
  // leave every member inert: no registrations, pending callbacks, or cppgc
  // or V8 handles.
  virtual void TearDownNative() = 0;

  // Runs at MicrotasksRunner shutdown while V8 can still run, before
  // Release(). Override it to act on the wrapper first, or to release in a
  // different order and emit afterwards; Release() is a no-op if it already
  // ran.
  virtual void OnShutdown() {}

  // Stops native callbacks from reaching the wrapper. Idempotent.
  virtual void DisconnectWrapper() = 0;

  static bool IsInSweepingFinalizer();

  // Wrappers may only be borrowed on the owning sequence and outside cppgc
  // finalizers, where creating a Persistent is not allowed.
  static void DCheckCanBorrowWrapper();

 private:
  enum class State { kActive, kReleasing, kReleased };

  struct DeleteNow {
    void operator()(NativePeerBase* peer) const;
  };
  using QueuedPeer = std::unique_ptr<NativePeerBase, DeleteNow>;

  static std::vector<QueuedPeer>& QueuedPeers();
  static void Queue(NativePeerBase* peer);
  static void ReleaseAndDeleteQueuedPeers();
  static void ReleaseAndDelete(std::vector<QueuedPeer> peers);

  // MicrotasksRunner::Observer
  void OnBeforeMicrotasksRunnerDispose() final;

  State state_ = State::kActive;
  bool observing_shutdown_ = false;
};

template <typename Wrapper>
class NativePeer;

// A stack-only borrow of a peer's wrapper, returned by NativePeer::wrapper().
//
// The borrow roots the wrapper with a cppgc::Persistent for as long as it is
// live, so the wrapper stays alive across calls that run JavaScript, trigger
// GC, or spin a nested run loop. It is only meant to live in the frame that
// took it, so this type cannot be copied, moved, heap allocated, or stored as
// a field, and therefore cannot be bound into a callback. To keep the wrapper
// beyond the frame, use gin::WrapPersistent or a gin::WeakCell instead.
//
// TODO(deepak1556): A raw pointer would be enough if V8's non-nestable tasks
// never ran inside nested run loops. V8 finishes GCs from those tasks without
// scanning the native stack, but Node's platform runs them as ordinary tasks
// and Electron drains them from a nestable UvRunOnce task, so they can run
// inside a nested run loop (for example a macOS menu popup) while this frame
// is live. Once the Node platform routes non-nestable tasks to a Chromium
// non-nestable task runner, drop the Persistent and hold a raw pointer.
template <typename Wrapper>
class WrapperRef final {
  CPPGC_STACK_ALLOCATED();

 public:
  WrapperRef(const WrapperRef&) = delete;
  WrapperRef& operator=(const WrapperRef&) = delete;

  explicit operator bool() const { return static_cast<bool>(wrapper_); }
  Wrapper* operator->() const {
    DCHECK(wrapper_);
    return wrapper_.Get();
  }

 private:
  friend class NativePeer<Wrapper>;

  explicit WrapperRef(Wrapper* wrapper) : wrapper_(wrapper) {}

  cppgc::Persistent<Wrapper> wrapper_;
};

template <typename Wrapper>
class NativePeer : public NativePeerBase {
 public:
  template <typename Peer>
  using Ptr = std::unique_ptr<Peer, Deleter>;

  template <typename Peer, typename... Args>
  static Ptr<Peer> Create(Args&&... args) {
    static_assert(std::derived_from<Peer, NativePeer<Wrapper>>,
                  "Peers must derive from NativePeer<Wrapper>");
    static_assert(!cppgc::IsGarbageCollectedOrMixinTypeV<Peer>,
                  "Native peers must not be cppgc managed");
    static_assert(!std::is_destructible_v<Peer>,
                  "Native peers must declare a non-public destructor so only "
                  "NativePeerBase::Deleter can delete them");
    return Ptr<Peer>(new Peer(std::forward<Args>(args)...));
  }

 protected:
  explicit NativePeer(Wrapper* wrapper) : wrapper_(wrapper) {
    static_assert(cppgc::IsGarbageCollectedTypeV<Wrapper>,
                  "NativePeer<Wrapper> requires a cppgc managed wrapper");
  }
  ~NativePeer() override = default;

  // Empty if the wrapper is unreachable or disconnected. A non-empty result
  // proves only that the wrapper is reachable, not that its native state is
  // live.
  WrapperRef<Wrapper> wrapper() const {
    DCheckCanBorrowWrapper();
    return WrapperRef<Wrapper>(wrapper_.Get());
  }

  void DisconnectWrapper() final { wrapper_.Clear(); }

 private:
  cppgc::WeakPersistent<Wrapper> wrapper_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NATIVE_PEER_H_
