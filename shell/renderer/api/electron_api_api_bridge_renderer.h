// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_RENDERER_API_ELECTRON_API_API_BRIDGE_RENDERER_H_
#define ELECTRON_SHELL_RENDERER_API_ELECTRON_API_API_BRIDGE_RENDERER_H_

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/memory/weak_ptr.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/promise.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-persistent-handle.h"

namespace electron::api {

// The renderer side of apiBridge for one RenderFrame.
//
// Holds the grants the browser passed to the current document, and the set it
// pushed for the document the next commit creates. Each grant is installed in
// its world, the page's main world or Electron's isolated world, as
// navigator.electron[name]: a frozen object of native functions, defined
// before any script runs in that world. navigator.electron itself exists only
// while the world has at least one API. No preload script or context bridge is
// involved, and nothing is added to the global object.
//
// This bookkeeping only decides what the page can see. The browser checks the
// grant, the frame and the frame's committed origin on every call.
class ApiBridgeRenderFrame final
    : public mojom::ElectronApiBridgeClient,
      public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<ApiBridgeRenderFrame> {
 public:
  explicit ApiBridgeRenderFrame(content::RenderFrame* render_frame);
  ~ApiBridgeRenderFrame() override;

  // disable copy
  ApiBridgeRenderFrame(const ApiBridgeRenderFrame&) = delete;
  ApiBridgeRenderFrame& operator=(const ApiBridgeRenderFrame&) = delete;

  // The ApiBridgeRenderFrame of the frame |context| belongs to, or null.
  static ApiBridgeRenderFrame* FromContext(v8::Local<v8::Context> context);

  // mojom::ElectronApiBridgeClient
  void SetPendingGrants(std::vector<mojom::ApiBridgeGrantPtr> grants) override;
  void UpdateGrants(const std::optional<std::string>& origin,
                    const std::vector<uint64_t>& revoked,
                    std::vector<mojom::ApiBridgeGrantPtr> added) override;
  void ResetGrants(std::vector<mojom::ApiBridgeGrantPtr> grants) override;
  void EmitEvent(uint64_t grant_id,
                 uint32_t member,
                 electron::SerializedValue args) override;
  void UpdateStore(uint64_t grant_id,
                   uint32_t member,
                   electron::SerializedValue value) override;

 private:
  struct Member {
    Member();
    ~Member();
    Member(Member&&);
    Member& operator=(Member&&);

    std::string name;
    mojom::ApiBridgeMemberKind kind = mojom::ApiBridgeMemberKind::kMethod;
    // The latest value of a store; empty for other kinds.
    std::vector<uint8_t> store_value;
  };

  struct Grant {
    Grant();
    ~Grant();
    Grant(Grant&&);
    Grant& operator=(Grant&&);

    uint64_t id = 0;
    std::string name;
    mojom::ApiBridgeWorld world = mojom::ApiBridgeWorld::kMain;
    std::vector<Member> members;
  };

  struct Listener {
    Listener(uint32_t id, v8::Global<v8::Function> function);
    ~Listener();
    Listener(Listener&&);
    Listener& operator=(Listener&&);

    uint32_t id;
    v8::Global<v8::Function> function;
  };

  // Page-side state of one member of an installed API.
  struct MemberState {
    MemberState();
    ~MemberState();
    MemberState(MemberState&&);
    MemberState& operator=(MemberState&&);

    std::vector<Listener> listeners;
    // The deserialized store value; empty until get() is called or after an
    // update.
    v8::Global<v8::Value> store_cache;
  };

  // A grant installed on a world's navigator.electron.
  struct InstalledApi {
    InstalledApi();
    ~InstalledApi();
    InstalledApi(InstalledApi&&);
    InstalledApi& operator=(InstalledApi&&);

    uint64_t grant_id = 0;
    std::string name;
    v8::Global<v8::Object> object;
    std::vector<MemberState> members;
  };

  // One world of the current document that can hold APIs.
  struct WorldState {
    WorldState();
    ~WorldState();

    // The world's script context, once it has been created.
    v8::Global<v8::Context> context;
    // The document |context| holds APIs for. A popup's first page reuses the
    // context of its initial empty document.
    uint64_t document = 0;
    // navigator.electron in |context|, while the world has an API.
    v8::Global<v8::Object> namespace_object;
    std::vector<InstalledApi> installed;
  };

  // A name on the main world's navigator.electron and the grant behind it.
  using InstalledNames = std::vector<std::pair<std::string, uint64_t>>;

  // content::RenderFrameObserver
  void DidCreateNewDocument() override;
  void DidClearWindowObject() override;
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;
  void WillReleaseScriptContext(v8::Isolate* const isolate,
                                v8::Local<v8::Context> context,
                                int32_t world_id) override;
  void OnDestruct() override;

  void BindClient(
      mojo::PendingAssociatedReceiver<mojom::ElectronApiBridgeClient> receiver);
  mojom::ElectronApiBridgeHost* GetHost();
  v8::Isolate* GetIsolate();
  // Whether Electron creates its isolated world in this frame: with
  // contextIsolation, in the main frame, and in iframes with
  // nodeIntegrationInSubFrames.
  bool HasIsolatedWorld();

  static Grant FromMojo(mojom::ApiBridgeGrantPtr grant);
  static Grant* FindGrant(std::vector<Grant>& grants, uint64_t grant_id);

  WorldState& GetWorld(mojom::ApiBridgeWorld world);
  // The world whose script context is |context|, or null.
  WorldState* WorldForContext(v8::Local<v8::Context> context);

  // Removes the |revoked| grants and adds the |added| ones, refreshing those
  // the document has already; then updates navigator.electron and fires
  // electronapichange for what changed in the main world.
  void ApplyChanges(const std::vector<uint64_t>& revoked,
                    std::vector<Grant> added);

  // Adds |grant| to navigator.electron in its world, if that world exists
  // yet. Replaces an API installed earlier under the same name.
  void Install(const Grant& grant);
  // Removes the API of |grant_id| from navigator.electron.
  void Uninstall(uint64_t grant_id);
  // navigator.electron in |world|, created with the first API. Defined again
  // with the same object if page script removed or replaced it.
  v8::MaybeLocal<v8::Object> GetOrCreateNamespace(WorldState& world);
  // Deletes navigator.electron from |world| once it holds no API.
  void RemoveNamespaceIfEmpty(WorldState& world);
  InstalledNames GetInstalledNames();
  void FireChangeEvents(const InstalledNames& before,
                        const InstalledNames& after);
  void Warn(const std::string& message);
  // The installed API of |grant_id| and the world it is in, or nulls.
  std::pair<WorldState*, InstalledApi*> FindInstalled(uint64_t grant_id);
  // The state of a member of an installed API of the world of |context|.
  MemberState* FindMemberState(v8::Local<v8::Context> context,
                               uint64_t grant_id,
                               uint32_t member);
  static v8::Local<v8::Object> Materialize(v8::Isolate* isolate,
                                           v8::Local<v8::Context> context,
                                           const Grant& grant);

  // Calls the listeners of an event (|bytes| is a serialized array of
  // arguments) or of a store (|bytes| is the new value).
  void Notify(uint64_t grant_id,
              uint32_t member,
              base::span<const uint8_t> bytes,
              bool is_event);

  // Settles the promise of an async call, unless the document that made the
  // call has been replaced meanwhile.
  static void OnCallReply(
      base::WeakPtr<ApiBridgeRenderFrame> self,
      uint64_t document,
      gin_helper::Promise<electron::SerializedValue> promise,
      mojom::ApiBridgeResultPtr result);

  // Native callbacks of the functions Materialize() creates. Their data is an
  // array of [grant id, member index, extra].
  static void OnMethodCall(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void OnSubscribe(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void OnUnsubscribe(const v8::FunctionCallbackInfo<v8::Value>& info);
  static void OnStoreGet(const v8::FunctionCallbackInfo<v8::Value>& info);

  // Grants passed to the current document.
  std::vector<Grant> active_;
  // Grants pushed for the document the next commit creates.
  std::optional<std::vector<Grant>> pending_;
  // Counts documents, so a reply to a call of an earlier document is dropped.
  uint64_t document_ = 0;

  // Indexed by mojom::ApiBridgeWorld.
  std::array<WorldState, 2> worlds_;
  uint32_t next_listener_id_ = 0;

  mojo::AssociatedReceiverSet<mojom::ElectronApiBridgeClient> client_receivers_;
  mojo::AssociatedRemote<mojom::ElectronApiBridgeHost> host_;

  base::WeakPtrFactory<ApiBridgeRenderFrame> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_RENDERER_API_ELECTRON_API_API_BRIDGE_RENDERER_H_
