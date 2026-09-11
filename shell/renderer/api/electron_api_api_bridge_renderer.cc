// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/renderer/api/electron_api_api_bridge_renderer.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "content/public/renderer/render_frame.h"
#include "gin/converter.h"
#include "shell/common/gin_converters/serialized_value_converter.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/node_includes.h"
#include "shell/common/serialized_value.h"
#include "shell/common/v8_util.h"
#include "shell/common/world_ids.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/common/web_preferences/web_preferences.h"
#include "third_party/blink/public/platform/scheduler/web_agent_group_scheduler.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "url/origin.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-promise.h"

namespace electron::api {

namespace {

// The message the browser uses for every refusal. A call through an API the
// document no longer has fails with it too, without leaving the renderer.
constexpr std::string_view kNotAvailableError =
    "This API is not available to this frame";

// Fired at the main world's window when its APIs change after it was created.
constexpr std::string_view kChangeEvent = "electronapichange";

// Slots of the array every native function carries as its data.
enum Slot : uint32_t { kGrantIdSlot = 0, kMemberSlot, kExtraSlot, kSlotCount };

struct CallData {
  uint64_t grant_id = 0;
  uint32_t member = 0;
  // 1 for a sync method, the listener id for an unsubscribe function.
  uint32_t extra = 0;
};

bool ReadCallData(const v8::FunctionCallbackInfo<v8::Value>& info,
                  CallData* out) {
  if (!info.Data()->IsArray())
    return false;
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Array> data = info.Data().As<v8::Array>();
  std::array<double, kSlotCount> slots;
  for (uint32_t i = 0; i < kSlotCount; ++i) {
    v8::Local<v8::Value> value;
    if (!data->Get(context, i).ToLocal(&value) || !value->IsNumber())
      return false;
    slots[i] = value.As<v8::Number>()->Value();
  }
  out->grant_id = static_cast<uint64_t>(slots[kGrantIdSlot]);
  out->member = static_cast<uint32_t>(slots[kMemberSlot]);
  out->extra = static_cast<uint32_t>(slots[kExtraSlot]);
  return true;
}

v8::Local<v8::Function> NewFunction(v8::Isolate* isolate,
                                    v8::Local<v8::Context> context,
                                    v8::FunctionCallback callback,
                                    uint64_t grant_id,
                                    uint32_t member,
                                    uint32_t extra,
                                    std::string_view name) {
  std::array<v8::Local<v8::Value>, kSlotCount> slots = {
      v8::Number::New(isolate, static_cast<double>(grant_id)),
      v8::Integer::NewFromUnsigned(isolate, member),
      v8::Integer::NewFromUnsigned(isolate, extra)};
  v8::Local<v8::Array> data =
      v8::Array::New(isolate, slots.data(), slots.size());
  v8::Local<v8::Function> function =
      v8::Function::New(context, callback, data, 0,
                        v8::ConstructorBehavior::kThrow)
          .ToLocalChecked();
  function->SetName(gin::StringToV8(isolate, name));
  return function;
}

// An error with |name| and |message|, created in the current context. The
// built-in error types the page can test with instanceof keep their type;
// any other name is set on an Error.
v8::Local<v8::Value> MakeError(v8::Isolate* isolate,
                               v8::Local<v8::Context> context,
                               std::string_view name,
                               std::string_view message) {
  v8::Local<v8::String> text = gin::StringToV8(isolate, message);
  if (name == "TypeError")
    return v8::Exception::TypeError(text);
  if (name == "RangeError")
    return v8::Exception::RangeError(text);
  if (name == "ReferenceError")
    return v8::Exception::ReferenceError(text);
  if (name == "SyntaxError")
    return v8::Exception::SyntaxError(text);
  v8::Local<v8::Value> error = v8::Exception::Error(text);
  if (!name.empty() && name != "Error") {
    // Where a subclass's instances find it, but on the object: not
    // enumerable.
    std::ignore = error.As<v8::Object>()->DefineOwnProperty(
        context, gin::StringToV8(isolate, "name"),
        gin::StringToV8(isolate, name), v8::DontEnum);
  }
  return error;
}

v8::Local<v8::Value> RefusalError(v8::Isolate* isolate,
                                  v8::Local<v8::Context> context) {
  return MakeError(isolate, context, "Error", kNotAvailableError);
}

v8::Local<v8::Promise> RejectedPromise(v8::Isolate* isolate,
                                       v8::Local<v8::Value> error) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  promise.Reject(error);
  return handle;
}

// Whether |object| has an own data property |key| whose value is |expected|.
// Reads the property descriptor, so no page getter runs.
bool HasOwnValue(v8::Isolate* isolate,
                 v8::Local<v8::Context> context,
                 v8::Local<v8::Object> object,
                 v8::Local<v8::String> key,
                 v8::Local<v8::Value> expected) {
  v8::Local<v8::String> value_key = gin::StringToV8(isolate, "value");
  v8::Local<v8::Value> descriptor;
  v8::Local<v8::Value> value;
  bool has_value = false;
  return object->GetOwnPropertyDescriptor(context, key).ToLocal(&descriptor) &&
         descriptor->IsObject() &&
         descriptor.As<v8::Object>()
             ->HasOwnProperty(context, value_key)
             .To(&has_value) &&
         has_value &&
         descriptor.As<v8::Object>()->Get(context, value_key).ToLocal(&value) &&
         value->StrictEquals(expected);
}

// The window's navigator in |context|. Reads whatever the page's `navigator`
// is, which only the page itself could have changed.
v8::MaybeLocal<v8::Object> GetNavigator(v8::Isolate* isolate,
                                        v8::Local<v8::Context> context) {
  v8::Local<v8::Value> navigator;
  if (!context->Global()
           ->Get(context, gin::StringToV8(isolate, "navigator"))
           .ToLocal(&navigator) ||
      !navigator->IsObject()) {
    return {};
  }
  return navigator.As<v8::Object>();
}

}  // namespace

ApiBridgeRenderFrame::Member::Member() = default;
ApiBridgeRenderFrame::Member::~Member() = default;
ApiBridgeRenderFrame::Member::Member(Member&&) = default;
ApiBridgeRenderFrame::Member& ApiBridgeRenderFrame::Member::operator=(
    Member&&) = default;

ApiBridgeRenderFrame::Grant::Grant() = default;
ApiBridgeRenderFrame::Grant::~Grant() = default;
ApiBridgeRenderFrame::Grant::Grant(Grant&&) = default;
ApiBridgeRenderFrame::Grant& ApiBridgeRenderFrame::Grant::operator=(Grant&&) =
    default;

ApiBridgeRenderFrame::Listener::Listener(uint32_t id,
                                         v8::Global<v8::Function> function)
    : id(id), function(std::move(function)) {}
ApiBridgeRenderFrame::Listener::~Listener() = default;
ApiBridgeRenderFrame::Listener::Listener(Listener&&) = default;
ApiBridgeRenderFrame::Listener& ApiBridgeRenderFrame::Listener::operator=(
    Listener&&) = default;

ApiBridgeRenderFrame::MemberState::MemberState() = default;
ApiBridgeRenderFrame::MemberState::~MemberState() = default;
ApiBridgeRenderFrame::MemberState::MemberState(MemberState&&) = default;
ApiBridgeRenderFrame::MemberState& ApiBridgeRenderFrame::MemberState::operator=(
    MemberState&&) = default;

ApiBridgeRenderFrame::InstalledApi::InstalledApi() = default;
ApiBridgeRenderFrame::InstalledApi::~InstalledApi() = default;
ApiBridgeRenderFrame::InstalledApi::InstalledApi(InstalledApi&&) = default;
ApiBridgeRenderFrame::InstalledApi&
ApiBridgeRenderFrame::InstalledApi::operator=(InstalledApi&&) = default;

ApiBridgeRenderFrame::WorldState::WorldState() = default;
ApiBridgeRenderFrame::WorldState::~WorldState() = default;

ApiBridgeRenderFrame::ApiBridgeRenderFrame(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<ApiBridgeRenderFrame>(render_frame) {
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<mojom::ElectronApiBridgeClient>(base::BindRepeating(
          &ApiBridgeRenderFrame::BindClient, base::Unretained(this)));
}

ApiBridgeRenderFrame::~ApiBridgeRenderFrame() = default;

// static
ApiBridgeRenderFrame* ApiBridgeRenderFrame::FromContext(
    v8::Local<v8::Context> context) {
  blink::WebLocalFrame* frame = blink::WebLocalFrame::FrameForContext(context);
  if (!frame)
    return nullptr;
  content::RenderFrame* render_frame =
      content::RenderFrame::FromWebFrame(frame);
  return render_frame ? Get(render_frame) : nullptr;
}

void ApiBridgeRenderFrame::SetPendingGrants(
    std::vector<mojom::ApiBridgeGrantPtr> grants) {
  std::vector<Grant> pending;
  pending.reserve(grants.size());
  for (auto& grant : grants)
    pending.push_back(FromMojo(std::move(grant)));
  pending_ = std::move(pending);
}

void ApiBridgeRenderFrame::UpdateGrants(
    const std::optional<std::string>& origin,
    const std::vector<uint64_t>& revoked,
    std::vector<mojom::ApiBridgeGrantPtr> added) {
  // Meant for the document this frame showed before a commit that the
  // browser had not seen yet.
  if (origin) {
    const url::Origin current =
        render_frame()->GetWebFrame()->GetSecurityOrigin();
    if (*origin != current.Serialize())
      return;
  }
  std::vector<Grant> grants;
  grants.reserve(added.size());
  for (auto& grant : added)
    grants.push_back(FromMojo(std::move(grant)));
  ApplyChanges(revoked, std::move(grants));
}

void ApiBridgeRenderFrame::ResetGrants(
    std::vector<mojom::ApiBridgeGrantPtr> grants) {
  std::vector<Grant> incoming;
  incoming.reserve(grants.size());
  for (auto& grant : grants)
    incoming.push_back(FromMojo(std::move(grant)));
  std::vector<uint64_t> revoked;
  for (const Grant& grant : active_) {
    if (!FindGrant(incoming, grant.id))
      revoked.push_back(grant.id);
  }
  ApplyChanges(revoked, std::move(incoming));
}

void ApiBridgeRenderFrame::EmitEvent(uint64_t grant_id,
                                     uint32_t member,
                                     electron::SerializedValue args) {
  const Grant* grant = FindGrant(active_, grant_id);
  if (!grant || member >= grant->members.size() ||
      grant->members[member].kind != mojom::ApiBridgeMemberKind::kEvent) {
    return;
  }
  Notify(grant_id, member, args.bytes(), /*is_event=*/true);
}

void ApiBridgeRenderFrame::UpdateStore(uint64_t grant_id,
                                       uint32_t member,
                                       electron::SerializedValue value) {
  auto update = [&](std::vector<Grant>& grants) {
    Grant* grant = FindGrant(grants, grant_id);
    if (!grant || member >= grant->members.size() ||
        grant->members[member].kind != mojom::ApiBridgeMemberKind::kStore) {
      return false;
    }
    auto bytes = value.bytes();
    grant->members[member].store_value.assign(bytes.begin(), bytes.end());
    return true;
  };
  if (pending_)
    update(*pending_);
  if (update(active_))
    Notify(grant_id, member, value.bytes(), /*is_event=*/false);
}

void ApiBridgeRenderFrame::DidCreateNewDocument() {
  // The grants the browser pushed ahead of this commit belong to the new
  // document; none of the old document's grants carry over. A document that
  // commits without a push (the initial empty document, a renderer-side
  // about:blank) starts with nothing.
  active_ = pending_ ? std::move(*pending_) : std::vector<Grant>();
  pending_.reset();
  ++document_;
}

void ApiBridgeRenderFrame::DidCreateScriptContext(
    v8::Local<v8::Context> context,
    int32_t world_id) {
  // The page's world, and the isolated world where Electron runs preload
  // scripts. Extension worlds and other isolated worlds get nothing.
  std::optional<mojom::ApiBridgeWorld> world;
  if (world_id == WorldIDs::MAIN_WORLD_ID)
    world = mojom::ApiBridgeWorld::kMain;
  else if (world_id == WorldIDs::ISOLATED_WORLD_ID && HasIsolatedWorld())
    world = mojom::ApiBridgeWorld::kIsolated;
  if (!world)
    return;
  // No script has run in |context| yet.
  WorldState& state = GetWorld(*world);
  state.context.Reset(GetIsolate(), context);
  state.document = document_;
  state.namespace_object.Reset();
  state.installed.clear();
  for (const Grant& grant : active_) {
    if (grant.world == *world)
      Install(grant);
  }
}

void ApiBridgeRenderFrame::DidClearWindowObject() {
  // Blink keeps the window, and with it the main world's script context, when
  // a window.open() popup replaces its initial empty document with a
  // same-origin page. No DidCreateScriptContext follows for the new document,
  // so its APIs go into the context that exists, after the old document's are
  // removed. ElectronRenderFrameObserver runs the preload script from this
  // notification in that case; this observer was added first, so the APIs are
  // there before the preload's first line.
  WorldState& world = GetWorld(mojom::ApiBridgeWorld::kMain);
  if (world.context.IsEmpty() || world.document == document_)
    return;
  world.document = document_;
  std::vector<uint64_t> old_grants;
  for (const InstalledApi& installed : world.installed)
    old_grants.push_back(installed.grant_id);
  for (uint64_t grant_id : old_grants)
    Uninstall(grant_id);
  RemoveNamespaceIfEmpty(world);
  for (const Grant& grant : active_) {
    if (grant.world == mojom::ApiBridgeWorld::kMain)
      Install(grant);
  }
}

void ApiBridgeRenderFrame::WillReleaseScriptContext(
    v8::Isolate* const isolate,
    v8::Local<v8::Context> context,
    int32_t world_id) {
  if (WorldState* world = WorldForContext(context)) {
    world->installed.clear();
    world->namespace_object.Reset();
    world->context.Reset();
  }
}

void ApiBridgeRenderFrame::OnDestruct() {
  delete this;
}

void ApiBridgeRenderFrame::BindClient(
    mojo::PendingAssociatedReceiver<mojom::ElectronApiBridgeClient> receiver) {
  client_receivers_.Add(this, std::move(receiver));
}

mojom::ElectronApiBridgeHost* ApiBridgeRenderFrame::GetHost() {
  if (!host_.is_bound() || !host_.is_connected()) {
    host_.reset();
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&host_);
  }
  return host_.get();
}

v8::Isolate* ApiBridgeRenderFrame::GetIsolate() {
  return render_frame()->GetWebFrame()->GetAgentGroupScheduler()->Isolate();
}

bool ApiBridgeRenderFrame::HasIsolatedWorld() {
  // The same rule ElectronRenderFrameObserver creates the world by.
  const auto& prefs = render_frame()->GetBlinkPreferences();
  return prefs.context_isolation && (render_frame()->IsMainFrame() ||
                                     prefs.node_integration_in_sub_frames);
}

// static
ApiBridgeRenderFrame::Grant ApiBridgeRenderFrame::FromMojo(
    mojom::ApiBridgeGrantPtr grant) {
  Grant out;
  out.id = grant->id;
  out.name = std::move(grant->name);
  out.world = grant->world;
  out.members.reserve(grant->members.size());
  for (auto& member : grant->members) {
    Member& m = out.members.emplace_back();
    m.name = std::move(member->name);
    m.kind = member->kind;
    if (member->store_value) {
      auto bytes = member->store_value->bytes();
      m.store_value.assign(bytes.begin(), bytes.end());
    }
  }
  return out;
}

// static
ApiBridgeRenderFrame::Grant* ApiBridgeRenderFrame::FindGrant(
    std::vector<Grant>& grants,
    uint64_t grant_id) {
  auto it = std::ranges::find(grants, grant_id, &Grant::id);
  return it == grants.end() ? nullptr : &*it;
}

ApiBridgeRenderFrame::WorldState& ApiBridgeRenderFrame::GetWorld(
    mojom::ApiBridgeWorld world) {
  return worlds_[world == mojom::ApiBridgeWorld::kMain ? 0 : 1];
}

ApiBridgeRenderFrame::WorldState* ApiBridgeRenderFrame::WorldForContext(
    v8::Local<v8::Context> context) {
  for (WorldState& world : worlds_) {
    if (!world.context.IsEmpty() && world.context == context)
      return &world;
  }
  return nullptr;
}

void ApiBridgeRenderFrame::ApplyChanges(const std::vector<uint64_t>& revoked,
                                        std::vector<Grant> added) {
  const InstalledNames before = GetInstalledNames();

  for (uint64_t grant_id : revoked) {
    auto matches = [grant_id](const Grant& grant) {
      return grant.id == grant_id;
    };
    std::erase_if(active_, matches);
    if (pending_)
      std::erase_if(*pending_, matches);
    // Objects the page kept a reference to stay behind; every member now
    // throws or rejects.
    Uninstall(grant_id);
  }

  // Stores of grants the document had already, whose value changed.
  std::vector<std::pair<uint64_t, uint32_t>> changed_stores;
  for (Grant& grant : added) {
    if (Grant* existing = FindGrant(active_, grant.id)) {
      for (uint32_t i = 0;
           i < existing->members.size() && i < grant.members.size(); ++i) {
        Member& member = existing->members[i];
        if (member.kind == mojom::ApiBridgeMemberKind::kStore &&
            member.store_value != grant.members[i].store_value) {
          member.store_value = std::move(grant.members[i].store_value);
          changed_stores.emplace_back(grant.id, i);
        }
      }
      continue;
    }
    active_.push_back(std::move(grant));
    Install(active_.back());
  }

  // After all changes, so a replaced API keeps the same navigator.electron.
  for (WorldState& world : worlds_)
    RemoveNamespaceIfEmpty(world);
  FireChangeEvents(before, GetInstalledNames());

  for (const auto& [grant_id, member] : changed_stores) {
    Grant* grant = FindGrant(active_, grant_id);
    if (grant && member < grant->members.size()) {
      Notify(grant_id, member, grant->members[member].store_value,
             /*is_event=*/false);
    }
  }
}

void ApiBridgeRenderFrame::Install(const Grant& grant) {
  WorldState& world = GetWorld(grant.world);
  if (world.context.IsEmpty() || FindInstalled(grant.id).second)
    return;
  // A grant that replaced another under the same name takes over the name.
  if (auto it =
          std::ranges::find(world.installed, grant.name, &InstalledApi::name);
      it != world.installed.end()) {
    Uninstall(it->grant_id);
  }

  v8::Isolate* isolate = GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = world.context.Get(isolate);
  v8::Context::Scope context_scope(context);
  // We get here from frame notifications and mojo messages, not from script.
  v8::MicrotasksScope microtasks_scope(
      isolate, context->GetMicrotaskQueue(),
      v8::MicrotasksScope::kDoNotRunMicrotasks);

  v8::Local<v8::Object> electron_namespace;
  if (!GetOrCreateNamespace(world).ToLocal(&electron_namespace))
    return;

  v8::Local<v8::Object> object = Materialize(isolate, context, grant);
  // Read-only, but configurable so a revoked or replaced API can be removed.
  v8::PropertyDescriptor descriptor(object, /*writable=*/false);
  descriptor.set_enumerable(true);
  descriptor.set_configurable(true);
  v8::TryCatch try_catch(isolate);
  if (!electron_namespace
           ->DefineProperty(context, gin::StringToV8(isolate, grant.name),
                            descriptor)
           .FromMaybe(false)) {
    // Only possible if page script locked the name down itself.
    Warn("apiBridge: the '" + grant.name +
         "' API could not be added to navigator.electron.");
    return;
  }

  InstalledApi& installed = world.installed.emplace_back();
  installed.grant_id = grant.id;
  installed.name = grant.name;
  installed.object.Reset(isolate, object);
  installed.members.resize(grant.members.size());
}

void ApiBridgeRenderFrame::Uninstall(uint64_t grant_id) {
  auto [world, installed] = FindInstalled(grant_id);
  if (!installed)
    return;

  v8::Isolate* isolate = GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = world->context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      isolate, context->GetMicrotaskQueue(),
      v8::MicrotasksScope::kDoNotRunMicrotasks);

  // Remove the property only while it still holds our object; the page may
  // have deleted or replaced it.
  if (!world->namespace_object.IsEmpty()) {
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Object> electron_namespace =
        world->namespace_object.Get(isolate);
    v8::Local<v8::String> key = gin::StringToV8(isolate, installed->name);
    if (HasOwnValue(isolate, context, electron_namespace, key,
                    installed->object.Get(isolate))) {
      std::ignore = electron_namespace->Delete(context, key);
    }
  }
  std::erase_if(world->installed, [grant_id](const InstalledApi& api) {
    return api.grant_id == grant_id;
  });
}

v8::MaybeLocal<v8::Object> ApiBridgeRenderFrame::GetOrCreateNamespace(
    WorldState& world) {
  v8::Isolate* isolate = GetIsolate();
  v8::Local<v8::Context> context = world.context.Get(isolate);
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Object> navigator;
  if (!GetNavigator(isolate, context).ToLocal(&navigator))
    return {};
  v8::Local<v8::String> key = gin::StringToV8(isolate, "electron");

  v8::Local<v8::Object> electron_namespace;
  if (!world.namespace_object.IsEmpty()) {
    electron_namespace = world.namespace_object.Get(isolate);
    if (HasOwnValue(isolate, context, navigator, key, electron_namespace))
      return electron_namespace;
    // Page script deleted or replaced it; define it again, with the same
    // object and the APIs it still holds.
  } else {
    electron_namespace = v8::Object::New(isolate);
  }
  // Not listed with navigator's own properties. Configurable, so that it can
  // be removed with the last API.
  v8::PropertyDescriptor descriptor(electron_namespace, /*writable=*/false);
  descriptor.set_enumerable(false);
  descriptor.set_configurable(true);
  if (!navigator->DefineProperty(context, key, descriptor).FromMaybe(false)) {
    Warn(
        "apiBridge: navigator.electron could not be created, so no APIs were "
        "added.");
    return {};
  }
  world.namespace_object.Reset(isolate, electron_namespace);
  return electron_namespace;
}

void ApiBridgeRenderFrame::RemoveNamespaceIfEmpty(WorldState& world) {
  if (!world.installed.empty() || world.namespace_object.IsEmpty() ||
      world.context.IsEmpty()) {
    return;
  }
  v8::Isolate* isolate = GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = world.context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(
      isolate, context->GetMicrotaskQueue(),
      v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Object> electron_namespace =
      world.namespace_object.Get(isolate);
  world.namespace_object.Reset();

  // A document without APIs shows no trace of apiBridge.
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Object> navigator;
  v8::Local<v8::String> key = gin::StringToV8(isolate, "electron");
  if (GetNavigator(isolate, context).ToLocal(&navigator) &&
      HasOwnValue(isolate, context, navigator, key, electron_namespace)) {
    std::ignore = navigator->Delete(context, key);
  }
}

ApiBridgeRenderFrame::InstalledNames ApiBridgeRenderFrame::GetInstalledNames() {
  InstalledNames names;
  for (const InstalledApi& installed :
       GetWorld(mojom::ApiBridgeWorld::kMain).installed) {
    names.emplace_back(installed.name, installed.grant_id);
  }
  return names;
}

void ApiBridgeRenderFrame::FireChangeEvents(const InstalledNames& before,
                                            const InstalledNames& after) {
  auto find = [](const InstalledNames& names, const std::string& name) {
    return std::ranges::find(names, name, &InstalledNames::value_type::first);
  };
  std::vector<std::pair<std::string, std::string_view>> changes;
  for (const auto& [name, grant_id] : before) {
    auto it = find(after, name);
    if (it == after.end())
      changes.emplace_back(name, "removed");
    else if (it->second != grant_id)
      changes.emplace_back(name, "replaced");
  }
  for (const auto& [name, grant_id] : after) {
    if (find(before, name) == before.end())
      changes.emplace_back(name, "added");
  }
  WorldState& world = GetWorld(mojom::ApiBridgeWorld::kMain);
  if (changes.empty() || world.context.IsEmpty())
    return;

  v8::Isolate* isolate = GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = world.context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(isolate, context->GetMicrotaskQueue(),
                                       v8::MicrotasksScope::kRunMicrotasks);
  // Sandboxed renderers, and pages without node integration, have no Node
  // environment in the main world.
  std::optional<node::CallbackScope> callback_scope;
  if (auto* env = node::Environment::GetCurrent(context))
    callback_scope.emplace(env, context->Global(), node::async_context{0, 0});

  // The page's own CustomEvent and dispatchEvent. If page script replaced
  // them, that only affects the page itself.
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> constructor;
  v8::Local<v8::Value> dispatch;
  if (!global->Get(context, gin::StringToV8(isolate, "CustomEvent"))
           .ToLocal(&constructor) ||
      !constructor->IsFunction() ||
      !global->Get(context, gin::StringToV8(isolate, "dispatchEvent"))
           .ToLocal(&dispatch) ||
      !dispatch->IsFunction()) {
    return;
  }
  for (const auto& [name, change] : changes) {
    v8::Local<v8::Object> detail = v8::Object::New(isolate);
    v8::Local<v8::Object> init = v8::Object::New(isolate);
    if (!detail
             ->CreateDataProperty(context, gin::StringToV8(isolate, "name"),
                                  gin::StringToV8(isolate, name))
             .FromMaybe(false) ||
        !detail
             ->CreateDataProperty(context, gin::StringToV8(isolate, "change"),
                                  gin::StringToV8(isolate, change))
             .FromMaybe(false) ||
        !detail->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen)
             .FromMaybe(false) ||
        !init->CreateDataProperty(context, gin::StringToV8(isolate, "detail"),
                                  detail)
             .FromMaybe(false)) {
      continue;
    }
    std::array<v8::Local<v8::Value>, 2> argv = {
        gin::StringToV8(isolate, kChangeEvent), init};
    v8::Local<v8::Object> event;
    if (!constructor.As<v8::Function>()
             ->NewInstance(context, argv.size(), argv.data())
             .ToLocal(&event)) {
      continue;
    }
    v8::Local<v8::Value> event_value = event;
    // Blink reports a listener that throws; the others still run.
    std::ignore =
        dispatch.As<v8::Function>()->Call(context, global, 1, &event_value);
  }
}

void ApiBridgeRenderFrame::Warn(const std::string& message) {
  render_frame()->GetWebFrame()->AddMessageToConsole(
      blink::WebConsoleMessage(blink::mojom::ConsoleMessageLevel::kWarning,
                               blink::WebString::FromUtf8(message)));
}

std::pair<ApiBridgeRenderFrame::WorldState*,
          ApiBridgeRenderFrame::InstalledApi*>
ApiBridgeRenderFrame::FindInstalled(uint64_t grant_id) {
  for (WorldState& world : worlds_) {
    auto it =
        std::ranges::find(world.installed, grant_id, &InstalledApi::grant_id);
    if (it != world.installed.end())
      return {&world, &*it};
  }
  return {nullptr, nullptr};
}

ApiBridgeRenderFrame::MemberState* ApiBridgeRenderFrame::FindMemberState(
    v8::Local<v8::Context> context,
    uint64_t grant_id,
    uint32_t member) {
  WorldState* world = WorldForContext(context);
  if (!world)
    return nullptr;
  auto it =
      std::ranges::find(world->installed, grant_id, &InstalledApi::grant_id);
  if (it == world->installed.end() || member >= it->members.size())
    return nullptr;
  return &it->members[member];
}

// static
v8::Local<v8::Object> ApiBridgeRenderFrame::Materialize(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const Grant& grant) {
  auto freeze = [&context](v8::Local<v8::Object> object) {
    object->SetIntegrityLevel(context, v8::IntegrityLevel::kFrozen).Check();
  };

  v8::Local<v8::Object> api = v8::Object::New(isolate);
  for (uint32_t i = 0; i < grant.members.size(); ++i) {
    const Member& member = grant.members[i];
    v8::Local<v8::Value> value;
    switch (member.kind) {
      case mojom::ApiBridgeMemberKind::kMethod:
      case mojom::ApiBridgeMemberKind::kSyncMethod: {
        const bool sync =
            member.kind == mojom::ApiBridgeMemberKind::kSyncMethod;
        value = NewFunction(isolate, context, &OnMethodCall, grant.id, i,
                            sync ? 1 : 0, member.name);
        break;
      }
      case mojom::ApiBridgeMemberKind::kEvent: {
        v8::Local<v8::Object> event = v8::Object::New(isolate);
        event
            ->CreateDataProperty(context, gin::StringToV8(isolate, "on"),
                                 NewFunction(isolate, context, &OnSubscribe,
                                             grant.id, i, 0, "on"))
            .Check();
        freeze(event);
        value = event;
        break;
      }
      case mojom::ApiBridgeMemberKind::kStore: {
        v8::Local<v8::Object> store = v8::Object::New(isolate);
        store
            ->CreateDataProperty(context, gin::StringToV8(isolate, "get"),
                                 NewFunction(isolate, context, &OnStoreGet,
                                             grant.id, i, 0, "get"))
            .Check();
        store
            ->CreateDataProperty(context, gin::StringToV8(isolate, "subscribe"),
                                 NewFunction(isolate, context, &OnSubscribe,
                                             grant.id, i, 0, "subscribe"))
            .Check();
        freeze(store);
        value = store;
        break;
      }
    }
    api->CreateDataProperty(context, gin::StringToV8(isolate, member.name),
                            value)
        .Check();
  }
  freeze(api);
  return api;
}

void ApiBridgeRenderFrame::Notify(uint64_t grant_id,
                                  uint32_t member,
                                  base::span<const uint8_t> bytes,
                                  bool is_event) {
  auto [world, installed] = FindInstalled(grant_id);
  if (!installed || member >= installed->members.size())
    return;
  MemberState& state = installed->members[member];
  if (!is_event)
    state.store_cache.Reset();
  if (state.listeners.empty())
    return;

  v8::Isolate* isolate = GetIsolate();
  v8::HandleScope handle_scope(isolate);
  // Snapshot the listeners: one may unsubscribe others while we dispatch.
  v8::LocalVector<v8::Function> listeners(isolate);
  for (auto& listener : state.listeners)
    listeners.push_back(listener.function.Get(isolate));

  v8::Local<v8::Context> context = world->context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks_scope(isolate, context->GetMicrotaskQueue(),
                                       v8::MicrotasksScope::kRunMicrotasks);
  // Sandboxed renderers, and pages without node integration, have no Node
  // environment in the main world.
  std::optional<node::CallbackScope> callback_scope;
  if (auto* env = node::Environment::GetCurrent(context))
    callback_scope.emplace(env, context->Global(), node::async_context{0, 0});

  v8::Local<v8::Value> value = DeserializeV8Value(isolate, bytes);
  v8::LocalVector<v8::Value> argv(isolate);
  if (is_event) {
    if (!value->IsArray())
      return;
    v8::Local<v8::Array> array = value.As<v8::Array>();
    for (uint32_t i = 0; i < array->Length(); ++i) {
      v8::Local<v8::Value> arg;
      if (!array->Get(context, i).ToLocal(&arg))
        return;
      argv.push_back(arg);
    }
  } else {
    // Listeners and later get() calls see the same object.
    state.store_cache.Reset(isolate, value);
    argv.push_back(value);
  }

  for (v8::Local<v8::Function> listener : listeners) {
    // A throwing listener is reported like any uncaught exception and does
    // not stop the others.
    v8::TryCatch try_catch(isolate);
    try_catch.SetVerbose(true);
    std::ignore = listener->Call(context, v8::Undefined(isolate), argv.size(),
                                 argv.data());
  }
}

// static
void ApiBridgeRenderFrame::OnCallReply(
    base::WeakPtr<ApiBridgeRenderFrame> self,
    uint64_t document,
    gin_helper::Promise<electron::SerializedValue> promise,
    mojom::ApiBridgeResultPtr result) {
  // The document that made the call is gone; its promise is dropped.
  if (!self || self->document_ != document)
    return;
  if (!result->is_error()) {
    promise.Resolve(result->get_value());
    return;
  }
  v8::Isolate* isolate = promise.isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = promise.GetContext();
  v8::Context::Scope context_scope(context);
  const mojom::ApiBridgeErrorPtr& error = result->get_error();
  promise.Reject(MakeError(isolate, context, error->name, error->message));
}

// static
void ApiBridgeRenderFrame::OnMethodCall(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  CallData data;
  if (!ReadCallData(info, &data))
    return;
  const bool sync = data.extra != 0;

  // Async methods always return a promise, so every failure rejects it; sync
  // methods throw.
  auto fail = [&](v8::Local<v8::Value> error) {
    if (sync)
      isolate->ThrowException(error);
    else
      info.GetReturnValue().Set(RejectedPromise(isolate, error));
  };

  // The function's own context is the world it was installed in, so a
  // same-origin iframe calling parent.navigator.electron is attributed to the
  // parent.
  ApiBridgeRenderFrame* self = FromContext(context);
  if (!self || !FindGrant(self->active_, data.grant_id))
    return fail(RefusalError(isolate, context));

  v8::LocalVector<v8::Value> values(isolate);
  values.reserve(info.Length());
  for (int i = 0; i < info.Length(); ++i)
    values.push_back(info[i]);
  v8::Local<v8::Array> args_array =
      v8::Array::New(isolate, values.data(), values.size());

  electron::SerializedValue args;
  if (sync) {
    // A serialization failure leaves the clone error pending.
    if (!SerializeV8Value(isolate, args_array, &args))
      return;
    mojom::ApiBridgeResultPtr result;
    if (!self->GetHost()->CallSync(data.grant_id, data.member, std::move(args),
                                   &result) ||
        !result) {
      return fail(MakeError(isolate, context, "Error",
                            "The main process did not answer"));
    }
    if (result->is_error()) {
      const mojom::ApiBridgeErrorPtr& error = result->get_error();
      return fail(MakeError(isolate, context, error->name, error->message));
    }
    info.GetReturnValue().Set(DeserializeV8Value(isolate, result->get_value()));
    return;
  }

  gin_helper::Promise<electron::SerializedValue> promise(isolate);
  info.GetReturnValue().Set(promise.GetHandle());
  {
    v8::TryCatch try_catch(isolate);
    if (!SerializeV8Value(isolate, args_array, &args)) {
      if (try_catch.HasCaught() && !try_catch.HasTerminated()) {
        v8::Local<v8::Value> exception = try_catch.Exception();
        try_catch.Reset();
        promise.Reject(exception);
      }
      return;
    }
  }
  self->GetHost()->Call(
      data.grant_id, data.member, std::move(args),
      base::BindOnce(&OnCallReply, self->weak_factory_.GetWeakPtr(),
                     self->document_, std::move(promise)));
}

// static
void ApiBridgeRenderFrame::OnSubscribe(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  CallData data;
  if (!ReadCallData(info, &data))
    return;
  if (info.Length() < 1 || !info[0]->IsFunction()) {
    gin_helper::ErrorThrower(isolate).ThrowTypeError(
        "The listener must be a function");
    return;
  }
  ApiBridgeRenderFrame* self = FromContext(context);
  MemberState* member =
      self ? self->FindMemberState(context, data.grant_id, data.member)
           : nullptr;
  if (!member) {
    isolate->ThrowException(RefusalError(isolate, context));
    return;
  }
  const uint32_t listener_id = ++self->next_listener_id_;
  member->listeners.push_back(
      {listener_id,
       v8::Global<v8::Function>(isolate, info[0].As<v8::Function>())});
  info.GetReturnValue().Set(NewFunction(isolate, context, &OnUnsubscribe,
                                        data.grant_id, data.member, listener_id,
                                        "unsubscribe"));
}

// static
void ApiBridgeRenderFrame::OnUnsubscribe(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  CallData data;
  if (!ReadCallData(info, &data))
    return;
  ApiBridgeRenderFrame* self = FromContext(context);
  if (!self)
    return;
  // Already gone if the API was revoked; unsubscribing twice is a no-op.
  if (MemberState* member =
          self->FindMemberState(context, data.grant_id, data.member)) {
    std::erase_if(member->listeners, [&data](const Listener& listener) {
      return listener.id == data.extra;
    });
  }
}

// static
void ApiBridgeRenderFrame::OnStoreGet(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  CallData data;
  if (!ReadCallData(info, &data))
    return;
  ApiBridgeRenderFrame* self = FromContext(context);
  const Grant* grant = self ? FindGrant(self->active_, data.grant_id) : nullptr;
  MemberState* member =
      self ? self->FindMemberState(context, data.grant_id, data.member)
           : nullptr;
  if (!grant || !member || data.member >= grant->members.size()) {
    isolate->ThrowException(RefusalError(isolate, context));
    return;
  }
  // The value lives in this process; get() never goes to the browser.
  if (member->store_cache.IsEmpty()) {
    member->store_cache.Reset(
        isolate, DeserializeV8Value(
                     isolate, base::span<const uint8_t>(
                                  grant->members[data.member].store_value)));
  }
  info.GetReturnValue().Set(member->store_cache.Get(isolate));
}

}  // namespace electron::api
