// Copyright (c) 2026 Microsoft Corporation.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/native_peer.h"

#include <tuple>
#include <utility>

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "v8/include/cppgc/heap-state.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-isolate.h"

namespace electron {

namespace {

struct QueueState {
  SEQUENCE_CHECKER(sequence_checker);
  bool release_scheduled = false;
  bool shutdown_started = false;
};

QueueState& GetQueueState() {
  static base::NoDestructor<QueueState> state;
  return *state;
}

}  // namespace

void NativePeerBase::Deleter::operator()(NativePeerBase* peer) const {
  if (peer)
    Queue(peer);
}

void NativePeerBase::DeleteNow::operator()(NativePeerBase* peer) const {
  delete peer;
}

NativePeerBase::NativePeerBase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(GetQueueState().sequence_checker);
}

NativePeerBase::~NativePeerBase() {
  CHECK(is_released());
  DCHECK(!observing_shutdown_);
}

void NativePeerBase::StartObservingShutdown() {
  DCHECK(is_active());
  if (observing_shutdown_)
    return;
  observing_shutdown_ = true;
  MicrotasksRunner::AddObserver(this);
}

void NativePeerBase::Release() {
  if (!is_active())
    return;
  DCHECK(!IsInSweepingFinalizer());
  state_ = State::kReleasing;
  if (observing_shutdown_) {
    observing_shutdown_ = false;
    MicrotasksRunner::RemoveObserver(this);
  }
  TearDownNative();
  DisconnectWrapper();
  state_ = State::kReleased;
}

void NativePeerBase::OnBeforeMicrotasksRunnerDispose() {
  observing_shutdown_ = false;
  OnShutdown();
  Release();
}

// static
bool NativePeerBase::IsInSweepingFinalizer() {
  v8::Isolate* isolate = v8::Isolate::TryGetCurrent();
  if (!isolate || !isolate->GetCppHeap())
    return false;
  return cppgc::subtle::HeapState::IsSweepingOnOwningThread(
      isolate->GetCppHeap()->GetHeapHandle());
}

// static
void NativePeerBase::DCheckCanBorrowWrapper() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(GetQueueState().sequence_checker);
  DCHECK(!IsInSweepingFinalizer());
}

// static
std::vector<NativePeerBase::QueuedPeer>& NativePeerBase::QueuedPeers() {
  static base::NoDestructor<std::vector<QueuedPeer>> peers;
  return *peers;
}

// static
void NativePeerBase::Queue(NativePeerBase* peer) {
  QueueState& state = GetQueueState();
  DCHECK_CALLED_ON_VALID_SEQUENCE(state.sequence_checker);
  QueuedPeers().emplace_back(peer);
  if (state.shutdown_started || state.release_scheduled ||
      !base::SequencedTaskRunner::HasCurrentDefault()) {
    return;
  }
  state.release_scheduled = true;
  base::SequencedTaskRunner::GetCurrentDefault()->PostNonNestableTask(
      FROM_HERE, base::BindOnce(&NativePeerBase::ReleaseAndDeleteQueuedPeers));
}

// static
void NativePeerBase::ReleaseAndDeleteQueuedPeers() {
  QueueState& state = GetQueueState();
  DCHECK_CALLED_ON_VALID_SEQUENCE(state.sequence_checker);
  state.release_scheduled = false;
  if (state.shutdown_started)
    return;
  // Releasing can run JavaScript, which can collect more wrappers and queue
  // more peers. Those schedule another task.
  ReleaseAndDelete(std::exchange(QueuedPeers(), {}));
}

// static
void NativePeerBase::ReleaseQueuedPeersForShutdown() {
  QueueState& state = GetQueueState();
  DCHECK_CALLED_ON_VALID_SEQUENCE(state.sequence_checker);
  state.shutdown_started = true;
  while (!QueuedPeers().empty())
    ReleaseAndDelete(std::exchange(QueuedPeers(), {}));
}

// static
void NativePeerBase::ReleaseAndDelete(std::vector<QueuedPeer> peers) {
  for (QueuedPeer& peer : peers) {
    peer->Release();
    peer.reset();
  }
}

// static
void NativePeerBase::DeleteQueuedPeersAfterIsolateDisposal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(GetQueueState().sequence_checker);
  for (QueuedPeer& peer : std::exchange(QueuedPeers(), {})) {
    if (peer->is_released())
      continue;
    // Releasing now could enter V8 after isolate disposal. Every peer that
    // observed shutdown has already been released, so this is a peer that
    // never called StartObservingShutdown(). Leak it rather than run its
    // teardown without an isolate.
    DUMP_WILL_BE_NOTREACHED();
    std::ignore = peer.release();
  }
}

}  // namespace electron
